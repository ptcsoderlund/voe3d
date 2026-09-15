// The DEFLATE encoder. See deflate.h for the contract, for why it cannot fail
// and for where the worst-case output size comes from.
//
// ONE BLOCK, FIXED HUFFMAN CODES, BFINAL SET, AND THAT IS THE WHOLE FORMAT SIDE.
// Not stored blocks: what this engine compresses is a screenshot, which is
// mostly flat colour, and flat colour is exactly where matching pays — a stored
// file is the pixel count plus five bytes per 65 535. Not dynamic Huffman: its
// tables are a second format to get wrong, for maybe a fifth fewer bytes on a
// file nobody transmits. The decoder next door reads all three, so the day a
// measurement asks for dynamic blocks nothing has to be read back.
//
// THE FIXED CODE, FROM RFC 1951 SECTION 3.2.6, IS THE ONE TABLE THAT IS NOT
// BELOW: literals 0-143 are eight bits starting at 0x30, 144-255 are nine bits
// starting at 0x190, 256-279 are seven bits starting at 0x00, and 280-287 are
// eight bits starting at 0xc0. Distances are all five bits and the code is the
// symbol. Huffman codes go out most-significant bit first; everything else in
// DEFLATE — the block header, every extra-bit field — goes out least-
// significant bit first. That disagreement is in the standard, not here, and it
// is why there are two write functions below.
//
// THE LENGTH AND DISTANCE TABLES ARE COPIED, NOT DERIVED, and they are the same
// two tables inflate.c carries. Lengths 3-258 are 29 symbols, 257 upwards: the
// first eight are one length each with no extra bits, then the span doubles
// every four symbols — one extra bit, then two, up to five — and then symbol 285
// is the single length 258 with no extra bits at all, which breaks the pattern.
// Distances 1-32768 are 30 symbols: the first four are one distance each, then
// every pair of symbols adds an extra bit. Deriving either by loop is how people
// get 285 and the pair structure wrong; copying them is how the rest of the
// world gets them right.
//
// CHANGING THE MATCHER DOES NOT CHANGE THE FORMAT, AND THAT IS THE LINE TO KEEP.
// Everything below `find_match` is a search: a three-byte hash into a chain of
// earlier positions, walked at most CHAIN_LIMIT deep, taking the longest match
// it finds. Make it lazy, make it look further, make it give up sooner — the
// stream stays legal as long as three rules hold: a match is at least MIN_MATCH
// and at most MAX_MATCH bytes, its distance is between one and WINDOW, and every
// position the encoder skips over is still inserted into the chains or later
// matches will silently stop finding it. Emitting only literals is always legal,
// which is why a broken matcher does not fail a round trip — it just stops
// compressing, and that is what the size checks in tests/deflate.c are for.
#include "deflate.h"

#include <base/assert.h>

#include <string.h>

// The format's own limits. WINDOW is DEFLATE's largest distance and MAX_MATCH
// its longest match; MIN_MATCH is three because a length/distance pair is never
// cheaper than three literals.
#define WINDOW 32768
#define WINDOW_MASK (WINDOW - 1)
#define MIN_MATCH 3
#define MAX_MATCH 258

// The search's own numbers, and the only ones that are a choice. The hash is
// 32 768 buckets over three bytes, and CHAIN_LIMIT is how many earlier
// candidates a position is allowed to cost: the whole encoder is O(count *
// CHAIN_LIMIT * MAX_MATCH) in the worst case, so the cost of raising it is
// visible here rather than hidden in a loop.
#define HASH_BITS 15
#define HASH_SIZE (1 << HASH_BITS)
#define CHAIN_LIMIT 32

// An empty chain slot. Positions are offsets into the input, so a real one can
// never be this.
#define NO_POSITION 0xffffffffu

// RFC 1951 section 3.2.5, the same tables inflate.c reads them out of.
static const uint16_t LENGTH_BASE[29] = {
	3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51,
	59, 67, 83, 99, 115, 131, 163, 195, 227, 258
};
static const uint8_t LENGTH_EXTRA[29] = {
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4,
	5, 5, 5, 5, 0
};
static const uint16_t DISTANCE_BASE[30] = {
	1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385,
	513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
};
static const uint8_t DISTANCE_EXTRA[30] = {
	0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10,
	10, 11, 11, 12, 12, 13, 13
};

// The output, a bit at a time. `value` holds fewer than eight bits between
// calls; whole bytes leave immediately, so `count` is always the length written
// so far and needs no flushing to be read.
//
// RUNNING OUT OF ROOM IS A PROGRAM BUG AND NOT A RETURN VALUE, which is the one
// place this file leans on deflate.h's worst-case arithmetic. The capacity was
// computed from the input length before a bit was written, so an overrun means
// that bound is wrong — an assert names it, rather than a failure path that
// every call site would have to check and that no input can reach.
struct writer {
	uint8_t *bytes;
	size_t capacity;
	size_t count;
	uint32_t value;
	int bits;
};

// Least-significant bit first: the block header and every extra-bit field.
static void put_bits(struct writer *writer, uint32_t value, int count)
{
	VOE_BASE_DEBUG_ASSERT(count >= 0 && count <= 16,
			      "DEFLATE never writes more than 16 bits at once");

	writer->value |= (value & ((1u << count) - 1u)) << writer->bits;
	writer->bits += count;

	while (writer->bits >= 8) {
		VOE_BASE_ASSERT(writer->count < writer->capacity,
				"the compressed worst case in deflate.h is wrong");
		writer->bytes[writer->count++] = (uint8_t)(writer->value & 0xff);
		writer->value >>= 8;
		writer->bits -= 8;
	}
}

// Most-significant bit first: Huffman codes, and only Huffman codes. Reversing
// here rather than keeping the codes pre-reversed is what lets the table in this
// file's header be read straight against RFC 1951.
static void put_code(struct writer *writer, uint32_t code, int count)
{
	uint32_t reversed = 0;
	int bit;

	for (bit = 0; bit < count; bit++)
		reversed = (reversed << 1) | ((code >> bit) & 1u);

	put_bits(writer, reversed, count);
}

// The fixed literal/length code. Symbols 286 and 287 exist in it and mean
// nothing, so they are never written.
static void put_symbol(struct writer *writer, int symbol)
{
	VOE_BASE_DEBUG_ASSERT(symbol >= 0 && symbol < 286,
			      "no such literal/length symbol");

	if (symbol < 144)
		put_code(writer, (uint32_t)(0x30 + symbol), 8);
	else if (symbol < 256)
		put_code(writer, (uint32_t)(0x190 + symbol - 144), 9);
	else if (symbol < 280)
		put_code(writer, (uint32_t)(symbol - 256), 7);
	else
		put_code(writer, (uint32_t)(0xc0 + symbol - 280), 8);
}

// Pad to a byte boundary with zeroes, which is what ends the last block.
static void flush(struct writer *writer)
{
	if (writer->bits > 0)
		put_bits(writer, 0, 8 - writer->bits);
}

static void put_byte(struct writer *writer, uint8_t byte)
{
	VOE_BASE_DEBUG_ASSERT(writer->bits == 0,
			      "a whole byte only goes out on a boundary");
	VOE_BASE_ASSERT(writer->count < writer->capacity,
			"the compressed worst case in deflate.h is wrong");
	writer->bytes[writer->count++] = byte;
}

// RFC 1950's checksum: two sums modulo 65521, the second over the first. The
// conditional subtractions are enough because `a` starts each step below 65521
// and grows by at most 255, and `b` by at most 65520.
static uint32_t adler32(const uint8_t *data, size_t count)
{
	uint32_t a = 1;
	uint32_t b = 0;
	size_t index;

	for (index = 0; index < count; index++) {
		a += data[index];
		if (a >= 65521u)
			a -= 65521u;
		b += a;
		if (b >= 65521u)
			b -= 65521u;
	}

	return (b << 16) | a;
}

static void put_length(struct writer *writer, size_t length)
{
	int symbol = 0;

	VOE_BASE_DEBUG_ASSERT(length >= MIN_MATCH && length <= MAX_MATCH,
			      "a match length outside the format");

	while (symbol < 28 && length >= LENGTH_BASE[symbol + 1])
		symbol++;

	put_symbol(writer, 257 + symbol);
	put_bits(writer, (uint32_t)(length - LENGTH_BASE[symbol]),
		 LENGTH_EXTRA[symbol]);
}

static void put_distance(struct writer *writer, size_t distance)
{
	int symbol = 0;

	VOE_BASE_DEBUG_ASSERT(distance >= 1 && distance <= WINDOW,
			      "a match distance outside the window");

	while (symbol < 29 && distance >= DISTANCE_BASE[symbol + 1])
		symbol++;

	put_code(writer, (uint32_t)symbol, 5);
	put_bits(writer, (uint32_t)(distance - DISTANCE_BASE[symbol]),
		 DISTANCE_EXTRA[symbol]);
}

// Three bytes into a bucket. Knuth's multiplicative hash, taking the high bits
// of the product, because the low bits of three packed bytes are the third byte
// and nothing else.
static uint32_t hash3(const uint8_t *at)
{
	uint32_t key = ((uint32_t)at[0] << 16) | ((uint32_t)at[1] << 8) | at[2];

	return (key * 2654435761u) >> (32 - HASH_BITS);
}

// The chains. `head` is the most recent position per bucket and `prev` the one
// before it, indexed by position modulo the window — so a chain walk has to
// check the distance itself rather than trust the link, and CHAIN_LIMIT bounds
// it whatever the links say.
struct chains {
	uint32_t *head;
	uint32_t *prev;
};

static void insert(struct chains *chains, const uint8_t *data, size_t count,
		   size_t position)
{
	uint32_t bucket;

	if (position + MIN_MATCH > count)
		return;

	bucket = hash3(data + position);
	chains->prev[position & WINDOW_MASK] = chains->head[bucket];
	chains->head[bucket] = (uint32_t)position;
}

// The longest match for `position`, or zero. `distance_out` is only written when
// a match is returned.
static size_t find_match(const struct chains *chains, const uint8_t *data,
			 size_t count, size_t position, size_t *distance_out)
{
	size_t limit = count - position;
	size_t best = 0;
	uint32_t candidate;
	int chain;

	if (limit > MAX_MATCH)
		limit = MAX_MATCH;
	if (limit < MIN_MATCH)
		return 0;

	candidate = chains->head[hash3(data + position)];

	for (chain = 0; chain < CHAIN_LIMIT; chain++) {
		size_t at = candidate;
		size_t distance;
		size_t length = 0;

		if (candidate == NO_POSITION || at >= position)
			break;

		distance = position - at;
		if (distance > WINDOW)
			break;

		while (length < limit &&
		       data[at + length] == data[position + length])
			length++;

		if (length >= MIN_MATCH && length > best) {
			best = length;
			*distance_out = distance;
			if (best == limit)
				break;
		}

		candidate = chains->prev[at & WINDOW_MASK];
	}

	return best;
}

void voe_assets_deflate(voe_base_arena *arena, const uint8_t *data,
			size_t count, voe_assets_deflate_result *out)
{
	struct voe_base_arena_mark mark;
	struct chains chains;
	struct writer writer = { 0 };
	uint32_t check;
	size_t position;

	VOE_BASE_ASSERT(arena != NULL, "compressing without an arena");
	VOE_BASE_ASSERT(data != NULL, "compressing from nothing");
	VOE_BASE_ASSERT(out != NULL, "compressing into nothing");
	VOE_BASE_ASSERT(count > 0, "compressing zero bytes");

	// The result is pushed first and the search tables after it, so
	// rewinding at the end gives back a quarter of a megabyte of chains and
	// leaves the caller's stream where it is.
	writer.capacity = count + count / 8 + 64;
	writer.bytes = voe_base_arena_push(arena, writer.capacity);

	mark = voe_base_arena_mark(arena);
	chains.head = voe_base_arena_push(arena,
					  HASH_SIZE * sizeof(*chains.head));
	chains.prev = voe_base_arena_push(arena, WINDOW * sizeof(*chains.prev));
	memset(chains.head, 0xff, HASH_SIZE * sizeof(*chains.head));
	memset(chains.prev, 0xff, WINDOW * sizeof(*chains.prev));

	// RFC 1950: method 8 with a 32 KiB window, no preset dictionary, and a
	// check byte chosen so that the pair is a multiple of 31. 0x78 0x01 is
	// the pair every encoder writes for this combination.
	put_byte(&writer, 0x78);
	put_byte(&writer, 0x01);

	// One block, and it is the last one: BFINAL, then BTYPE 01 for fixed.
	put_bits(&writer, 1, 1);
	put_bits(&writer, 1, 2);

	position = 0;
	while (position < count) {
		size_t distance = 0;
		size_t length = find_match(&chains, data, count, position,
					   &distance);

		if (length >= MIN_MATCH) {
			size_t index;

			put_length(&writer, length);
			put_distance(&writer, distance);

			// Every position the match covers still goes into the
			// chains — skipping them is how a matcher stops seeing
			// what it already emitted.
			for (index = 0; index < length; index++)
				insert(&chains, data, count, position + index);
			position += length;
			continue;
		}

		put_symbol(&writer, data[position]);
		insert(&chains, data, count, position);
		position++;
	}

	put_symbol(&writer, 256);
	flush(&writer);

	check = adler32(data, count);
	put_byte(&writer, (uint8_t)(check >> 24));
	put_byte(&writer, (uint8_t)(check >> 16));
	put_byte(&writer, (uint8_t)(check >> 8));
	put_byte(&writer, (uint8_t)check);

	voe_base_arena_rewind(arena, mark);

	out->bytes = writer.bytes;
	out->count = writer.count;
}
