// THE COMPRESSOR AGAINST THE DECOMPRESSOR, WHICH IS ONE HALF OF THE PROOF AND
// NOT ALL OF IT. Every case below compresses a buffer and inflates it with
// src/inflate.c, so a round trip that comes back byte for byte says the two
// agree — it does not say either of them agrees with the rest of the world. The
// other half was done by hand once and is not automated: python3's zlib read a
// stream this encoder wrote and gave back the bytes that went in.
//
// A ROUND TRIP ALONE WOULD PASS WITH THE MATCHER SWITCHED OFF. Emitting nothing
// but literals is legal DEFLATE and inflates perfectly, so a matcher that
// silently found nothing would look exactly like a working one. That is what the
// size checks are for: every case whose input is repetitive also checks the
// compressed form is smaller than the input, and the maximum-distance case
// checks one buffer against a second that differs only in being unmatchable.
//
// The sizes here are chosen against the format's own edges — the 32 768-byte
// window, the 258-byte longest match — and against the chain indexing, which
// wraps at the window and would collide silently.
#include "../src/deflate.h"

#include "../src/inflate.h"

#include <base/arena.h>
#include <testing/test.h>

#include <string.h>

// A repeatable sequence with no structure a matcher can use. The top bits of a
// linear congruential generator, because its low bits alternate.
static void fill_pseudorandom(uint8_t *bytes, size_t count, uint32_t seed)
{
	uint32_t state = seed;
	size_t index;

	for (index = 0; index < count; index++) {
		state = state * 1103515245u + 12345u;
		bytes[index] = (uint8_t)(state >> 16);
	}
}

// Compress, inflate the result, and say whether the bytes came back identical.
// The compressed length goes into `compressed` so a caller can check the
// matcher did something. The arena is left as it was found.
static bool round_trip(voe_base_arena *arena, const uint8_t *data, size_t count,
		       size_t *compressed)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
	voe_assets_deflate_result result;
	voe_base_error error = VOE_BASE_OK;
	uint8_t *back;
	bool same;

	voe_assets_deflate(arena, data, count, &result);
	*compressed = result.count;

	back = voe_base_arena_push(arena, count);
	same = voe_assets_inflate(result.bytes, result.count, back, count,
				  &error) &&
	       memcmp(back, data, count) == 0;

	voe_base_arena_rewind(arena, mark);
	return same;
}

static void a_single_byte_round_trips(void)
{
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);
	const uint8_t byte = 'Q';
	size_t compressed = 0;

	VOE_TEST_CHECK(round_trip(arena, &byte, 1, &compressed));
	voe_base_arena_destroy(arena);
}

// A hundred thousand of one byte is one literal and then matches of the longest
// length the format has, over and over.
static void a_long_run_round_trips(void)
{
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);
	const size_t count = 100000;
	uint8_t *data = voe_base_arena_push(arena, count);
	size_t compressed = 0;

	memset(data, 'Z', count);

	VOE_TEST_CHECK(round_trip(arena, data, count, &compressed));
	VOE_TEST_CHECK(compressed < count);

	voe_base_arena_destroy(arena);
}

// Nothing to match: the whole buffer goes out as literals, and the stream is a
// little larger than the input, which is what fixed Huffman costs.
static void bytes_with_no_repeats_round_trip(void)
{
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);
	const size_t count = 8192;
	uint8_t *data = voe_base_arena_push(arena, count);
	size_t compressed = 0;

	fill_pseudorandom(data, count, 1u);

	VOE_TEST_CHECK(round_trip(arena, data, count, &compressed));

	voe_base_arena_destroy(arena);
}

// A 37-byte pattern over 200 kB: the chains are walked the whole way and the
// window fills several times over.
static void a_repeating_pattern_round_trips(void)
{
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);
	const size_t count = 200000;
	uint8_t *data = voe_base_arena_push(arena, count);
	uint8_t pattern[37];
	size_t compressed = 0;
	size_t index;

	fill_pseudorandom(pattern, sizeof(pattern), 2u);
	for (index = 0; index < count; index++)
		data[index] = pattern[index % sizeof(pattern)];

	VOE_TEST_CHECK(round_trip(arena, data, count, &compressed));
	VOE_TEST_CHECK(compressed < count);

	voe_base_arena_destroy(arena);
}

// THE FARTHEST A MATCH MAY REACH. The marker bytes at the start come back at
// offset 32768 exactly, which is the last distance the format allows —
// one more and the encoder must emit literals instead.
//
// A round trip cannot see the difference, so the measurement is a second buffer
// identical but for its tail, which matches nothing. The matched one has to come
// out smaller; if it does not, the long distance was refused or never searched.
static void a_match_at_the_maximum_distance_is_taken(void)
{
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);
	const uint8_t marker[] = "MAXIMUM_DISTANCE";
	const size_t count = 32768 + sizeof(marker);
	uint8_t *matched = voe_base_arena_push(arena, count);
	uint8_t *unmatched = voe_base_arena_push(arena, count);
	size_t matched_size = 0;
	size_t unmatched_size = 0;

	fill_pseudorandom(matched, count, 3u);
	memcpy(matched, marker, sizeof(marker));
	memcpy(matched + 32768, marker, sizeof(marker));

	memcpy(unmatched, matched, count);
	fill_pseudorandom(unmatched + 32768, sizeof(marker), 4u);

	VOE_TEST_CHECK(round_trip(arena, matched, count, &matched_size));
	VOE_TEST_CHECK(round_trip(arena, unmatched, count, &unmatched_size));
	VOE_TEST_CHECK(matched_size < unmatched_size);

	voe_base_arena_destroy(arena);
}

// THE LONGEST A MATCH MAY BE. The pattern repeats after 300 bytes, so the match
// at offset 300 would run to 300 and is cut to 258; the 42 bytes left over are a
// second match behind it. An encoder that let a length past 258 through writes a
// symbol the decoder does not have.
static void a_match_at_the_maximum_length_round_trips(void)
{
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);
	const size_t pattern = 300;
	const size_t count = pattern * 2;
	uint8_t *data = voe_base_arena_push(arena, count);
	size_t compressed = 0;

	fill_pseudorandom(data, pattern, 5u);
	memcpy(data + pattern, data, pattern);

	VOE_TEST_CHECK(round_trip(arena, data, count, &compressed));
	VOE_TEST_CHECK(compressed < count);

	voe_base_arena_destroy(arena);
}

// One byte past the window is where the chain array wraps: position 32768 and
// position 0 share a slot in `prev`, so a link that is trusted rather than
// checked sends the search backwards into the wrong place.
static void one_byte_past_the_window_round_trips(void)
{
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);
	const size_t count = 32769;
	uint8_t *data = voe_base_arena_push(arena, count);
	size_t compressed = 0;

	fill_pseudorandom(data, count, 6u);

	VOE_TEST_CHECK(round_trip(arena, data, count, &compressed));

	voe_base_arena_destroy(arena);
}

int main(void)
{
	a_single_byte_round_trips();
	a_long_run_round_trips();
	bytes_with_no_repeats_round_trip();
	a_repeating_pattern_round_trips();
	a_match_at_the_maximum_distance_is_taken();
	a_match_at_the_maximum_length_round_trips();
	one_byte_past_the_window_round_trips();
	return voe_test_result();
}
