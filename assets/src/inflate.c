// The DEFLATE decoder. See inflate.h for the contract and for why the output
// size is an input rather than something discovered.
//
// EVERY LOOP IN HERE IS BOUNDED BY THE OUTPUT BUFFER OR BY THE INPUT LENGTH, AND
// THAT IS DELIBERATE RATHER THAN INCIDENTAL. A compressed stream is the most
// hostile thing this engine reads: it is a program, written by whoever made the
// file, for a machine that copies bytes. The three ways it attacks are running
// the input off the end, writing the output off the end, and copying from before
// the start of the output — so the bit reader refuses to read past its input, no
// write happens without room having been checked first, and a back-reference
// that points further back than what has been written is refused.
//
// IT DECODES A BIT AT A TIME AND NOT THROUGH A LOOKUP TABLE. The fast decoders
// build a table indexed by the next nine or so bits; that is worth doing when
// something decompresses a lot, and nothing here does yet — one texture at
// startup. The canonical-code walk below is a dozen lines that can be read
// against RFC 1951 section 3.2.2, and it is the version that is obviously right.
// The card that loads enough images for this to matter is the card that changes
// it, with a measurement in it.
#include "inflate.h"

#include <base/assert.h>

#include <string.h>

// The longest a Huffman code may be in DEFLATE, and the largest alphabet: 288
// literal/length symbols. The distance alphabet has 30 and the code-length
// alphabet 19, and both borrow this array rather than have one each.
#define MAX_BITS 15
#define MAX_SYMBOLS 288

// Reading bits, least-significant first, which is the order DEFLATE packs them
// and the opposite of the order its Huffman codes are written in. That
// disagreement is in the standard, not here.
//
// RUNNING OUT OF INPUT IS A STICKY FLAG AND NOT A RETURN VALUE. A bit reader
// that returned a failure from every read would put a check on every one of the
// forty-odd call sites below, and the one that got forgotten would be the bug.
// Instead an exhausted reader hands back zeroes and remembers, every caller runs
// to a natural stopping point on those zeroes, and `overrun` is tested once at
// the end of each block. Zeroes cannot loop forever: a zero length is a zero
// length, and the output-full check ends it regardless.
struct bits {
	const uint8_t *bytes;
	size_t size;
	size_t next;
	uint32_t value;
	int count;
	bool overrun;
};

static uint32_t bits_take(struct bits *stream, int wanted)
{
	uint32_t value;

	VOE_BASE_DEBUG_ASSERT(wanted >= 0 && wanted <= 24,
			      "DEFLATE never asks for more than 24 bits at once");

	while (stream->count < wanted) {
		if (stream->next >= stream->size) {
			stream->overrun = true;
			return 0;
		}
		stream->value |= (uint32_t)stream->bytes[stream->next++]
				 << stream->count;
		stream->count += 8;
	}

	value = stream->value & ((1u << wanted) - 1u);
	stream->value >>= wanted;
	stream->count -= wanted;
	return value;
}

// A canonical Huffman code, as a count of codes per length and the symbols in
// canonical order. This is RFC 1951's own representation and it is why building
// one is a histogram rather than a tree.
struct huffman {
	uint16_t count[MAX_BITS + 1];
	uint16_t symbol[MAX_SYMBOLS];
};

// Build from a list of code lengths, and refuse only a set that cannot be a code
// at all.
//
// OVER-SUBSCRIBED IS REFUSED AND INCOMPLETE IS NOT, WHICH LOOKS LAX AND IS NOT.
// More codes of a given length than the tree has room for describes no code at
// all, so it is refused. A code with unused patterns left in it — incomplete —
// is legal, common, and unavoidable: DEFLATE's own fixed distance code is 30
// symbols in a five-bit space and leaves patterns 30 and 31 unassigned, so
// refusing incomplete codes refuses every fixed block. Encoders also emit an
// empty or single-symbol distance alphabet for a block that has no matches in
// it.
//
// WHAT MAKES THAT SAFE IS huffman_decode AND NOT THIS FUNCTION. The walk below
// only ever returns symbol[index + (code - first)] having established that
// code - first is less than the number of symbols at that length, so the index
// stays inside the part of the array that was actually filled in. A pattern that
// was never assigned runs off the end of the longest code and comes back as -1,
// and every one of the four call sites treats that as a malformed stream. So an
// incomplete table cannot produce a symbol that is not in it, and completeness
// was never what was protecting anything.
[[nodiscard]] static bool huffman_build(struct huffman *table,
					const uint8_t *lengths, int count)
{
	int left;
	int length;
	int symbol;
	uint16_t offsets[MAX_BITS + 1];

	memset(table->count, 0, sizeof(table->count));
	for (symbol = 0; symbol < count; symbol++)
		table->count[lengths[symbol]]++;

	// Kraft's inequality, walked one length at a time. `left` is how many
	// code patterns of the current length remain unclaimed, and it going
	// negative means more codes were asked for than exist. Ending above
	// zero is an incomplete code, which is allowed — see above.
	left = 1;
	for (length = 1; length <= MAX_BITS; length++) {
		left <<= 1;
		left -= table->count[length];
		if (left < 0)
			return false;
	}

	offsets[1] = 0;
	for (length = 1; length < MAX_BITS; length++)
		offsets[length + 1] = (uint16_t)(offsets[length] +
						 table->count[length]);

	for (symbol = 0; symbol < count; symbol++)
		if (lengths[symbol] != 0)
			table->symbol[offsets[lengths[symbol]]++] =
				(uint16_t)symbol;

	return true;
}

// Walk the canonical code one bit at a time. Returns the symbol, or -1 for a
// pattern the table does not contain — which an exhausted reader also produces,
// because its zeroes eventually fall off the end of the longest code.
static int huffman_decode(struct bits *stream, const struct huffman *table)
{
	int code = 0;
	int first = 0;
	int index = 0;
	int length;

	for (length = 1; length <= MAX_BITS; length++) {
		code |= (int)bits_take(stream, 1);
		if (code - first < table->count[length])
			return table->symbol[index + (code - first)];
		index += table->count[length];
		first = (first + table->count[length]) << 1;
		code <<= 1;
	}

	return -1;
}

// RFC 1951 section 3.2.5. Symbol 285 is 258 with no extra bits, which breaks the
// pattern the rest of the table follows; copying the table is how everyone gets
// that right and deriving it is how they get it wrong.
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

// Where a block's decoded bytes go. `written` is both the output position and
// the limit a back-reference may reach behind.
struct output {
	uint8_t *bytes;
	size_t capacity;
	size_t written;
};

// One literal/length + distance block, whichever way its tables were built.
[[nodiscard]] static bool inflate_block(struct bits *stream,
					struct output *out,
					const struct huffman *literals,
					const struct huffman *distances)
{
	for (;;) {
		int symbol = huffman_decode(stream, literals);

		if (symbol < 0 || stream->overrun)
			return false;

		if (symbol < 256) {
			if (out->written >= out->capacity)
				return false;
			out->bytes[out->written++] = (uint8_t)symbol;
			continue;
		}

		// 256 ends the block. Everything above it is a length.
		if (symbol == 256)
			return true;

		symbol -= 257;
		if (symbol >= 29)
			return false;

		size_t length = LENGTH_BASE[symbol] +
				bits_take(stream, LENGTH_EXTRA[symbol]);

		int distance_symbol = huffman_decode(stream, distances);
		if (distance_symbol < 0 || distance_symbol >= 30)
			return false;

		size_t distance = DISTANCE_BASE[distance_symbol] +
				  bits_take(stream, DISTANCE_EXTRA[distance_symbol]);

		if (stream->overrun)
			return false;

		// THE TWO CHECKS THAT MATTER. Reaching further back than what
		// has been written reads uninitialised memory and leaks it into
		// the picture; writing more than there is room for is the
		// overflow. Neither is reachable from a well-formed file, and
		// both are one line in a hostile one.
		if (distance == 0 || distance > out->written)
			return false;
		if (length > out->capacity - out->written)
			return false;

		// Byte at a time, and NOT memcpy, because an overlapping copy
		// is not a mistake here — it is how DEFLATE encodes a run. A
		// distance of one and a length of a hundred means "repeat that
		// byte a hundred times", and memcpy of overlapping regions is
		// undefined where this is defined.
		for (size_t i = 0; i < length; i++) {
			out->bytes[out->written] =
				out->bytes[out->written - distance];
			out->written++;
		}
	}
}

// The fixed tables of RFC 1951 section 3.2.6, built on demand rather than kept
// as constants: it is a hundred bytes of lengths and a build that takes no
// measurable time, against two tables that would have to be kept correct by eye.
[[nodiscard]] static bool build_fixed(struct huffman *literals,
				      struct huffman *distances)
{
	uint8_t lengths[MAX_SYMBOLS];
	int symbol;

	for (symbol = 0; symbol < 144; symbol++)
		lengths[symbol] = 8;
	for (; symbol < 256; symbol++)
		lengths[symbol] = 9;
	for (; symbol < 280; symbol++)
		lengths[symbol] = 7;
	for (; symbol < 288; symbol++)
		lengths[symbol] = 8;
	if (!huffman_build(literals, lengths, 288))
		return false;

	for (symbol = 0; symbol < 30; symbol++)
		lengths[symbol] = 5;
	return huffman_build(distances, lengths, 30);
}

// The order the code-length code's own lengths arrive in. It is not sorted, and
// the reason is that the ones most likely to be zero are put last so that HCLEN
// can cut them off.
static const uint8_t LENGTH_ORDER[19] = {
	16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

// A dynamic block's two tables, which are themselves Huffman-coded.
[[nodiscard]] static bool build_dynamic(struct bits *stream,
					struct huffman *literals,
					struct huffman *distances)
{
	struct huffman code_lengths;
	uint8_t lengths[MAX_SYMBOLS + 32];
	int literal_count = (int)bits_take(stream, 5) + 257;
	int distance_count = (int)bits_take(stream, 5) + 1;
	int code_count = (int)bits_take(stream, 4) + 4;
	int index;

	// 286 and 30 are the alphabet sizes; a header claiming more is
	// malformed rather than merely unusual.
	if (literal_count > 286 || distance_count > 30)
		return false;

	memset(lengths, 0, sizeof(lengths));
	for (index = 0; index < code_count; index++)
		lengths[LENGTH_ORDER[index]] = (uint8_t)bits_take(stream, 3);
	if (stream->overrun)
		return false;
	if (!huffman_build(&code_lengths, lengths, 19))
		return false;

	// The two alphabets are read as one run, which is what lets a repeat
	// straddle the boundary between them. That is legal and encoders do it.
	index = 0;
	while (index < literal_count + distance_count) {
		int symbol = huffman_decode(stream, &code_lengths);
		int repeat;
		uint8_t value = 0;

		if (symbol < 0 || stream->overrun)
			return false;

		if (symbol < 16) {
			lengths[index++] = (uint8_t)symbol;
			continue;
		}

		if (symbol == 16) {
			// Repeat the previous length, so there must be one.
			if (index == 0)
				return false;
			value = lengths[index - 1];
			repeat = 3 + (int)bits_take(stream, 2);
		} else if (symbol == 17) {
			repeat = 3 + (int)bits_take(stream, 3);
		} else {
			repeat = 11 + (int)bits_take(stream, 7);
		}

		if (index + repeat > literal_count + distance_count)
			return false;
		while (repeat-- > 0)
			lengths[index++] = value;
	}

	// A literal alphabet with no end-of-block symbol never terminates.
	if (lengths[256] == 0)
		return false;

	if (!huffman_build(literals, lengths, literal_count))
		return false;
	return huffman_build(distances, lengths + literal_count,
			     distance_count);
}

bool voe_assets_inflate(const uint8_t *bytes, size_t size, uint8_t *out,
			size_t capacity, voe_base_error *error)
{
	struct bits stream = { .bytes = bytes, .size = size };
	struct output output = { .bytes = out, .capacity = capacity };
	bool final = false;

	VOE_BASE_DEBUG_ASSERT(bytes != NULL, "inflating from nothing");
	VOE_BASE_DEBUG_ASSERT(out != NULL, "inflating into nothing");

	// The zlib wrapper: two header bytes, then the DEFLATE stream, then an
	// Adler-32 nobody checks — see the end of this function.
	if (size < 2)
		goto malformed;
	{
		uint8_t cmf = bytes[0];
		uint8_t flg = bytes[1];

		// Method 8 is DEFLATE and there is no method 9. The window size
		// is not checked because the output buffer is the only window
		// this decoder has and it is already the right size.
		if ((cmf & 0x0f) != 8)
			goto malformed;
		if (((uint32_t)cmf * 256 + flg) % 31 != 0)
			goto malformed;
		// A preset dictionary is a dictionary this decoder does not
		// have, so the stream cannot be decoded at all. PNG forbids it.
		if (flg & 0x20)
			goto malformed;
		stream.next = 2;
	}

	while (!final) {
		struct huffman literals;
		struct huffman distances;
		uint32_t type;

		final = bits_take(&stream, 1) != 0;
		type = bits_take(&stream, 2);
		if (stream.overrun)
			goto malformed;

		if (type == 0) {
			// Stored: byte-aligned, a length and its complement.
			uint32_t length;
			uint32_t check;

			stream.value = 0;
			stream.count = 0;
			if (stream.next + 4 > stream.size)
				goto malformed;
			length = (uint32_t)stream.bytes[stream.next] |
				 ((uint32_t)stream.bytes[stream.next + 1] << 8);
			check = (uint32_t)stream.bytes[stream.next + 2] |
				((uint32_t)stream.bytes[stream.next + 3] << 8);
			stream.next += 4;
			if (length != (~check & 0xffff))
				goto malformed;
			if (stream.next + length > stream.size)
				goto malformed;
			if (length > output.capacity - output.written)
				goto malformed;
			memcpy(output.bytes + output.written,
			       stream.bytes + stream.next, length);
			stream.next += length;
			output.written += length;
			continue;
		}

		if (type == 1) {
			if (!build_fixed(&literals, &distances))
				goto malformed;
		} else if (type == 2) {
			if (!build_dynamic(&stream, &literals, &distances))
				goto malformed;
		} else {
			// Type 3 is reserved and means the stream is not one.
			goto malformed;
		}

		if (!inflate_block(&stream, &output, &literals, &distances))
			goto malformed;
	}

	// Exactly the promised size, or the header that said so was wrong.
	if (output.written != capacity)
		goto malformed;

	// THE ADLER-32 IS NOT CHECKED, AND THAT IS A DECISION RATHER THAN AN
	// OMISSION. It guards against a stream that decompressed without
	// complaint into the wrong bytes, which needs a corruption that leaves
	// every Huffman code, every distance and the total length intact — and
	// against that, PNG has already put a CRC-32 on each chunk, which is
	// checked. Two checksums over the same bytes buys one of them.
	return true;

malformed:
	if (error != NULL)
		*error = VOE_BASE_ERROR_MALFORMED;
	return false;
}
