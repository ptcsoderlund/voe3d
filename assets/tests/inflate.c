// DEFLATE, INCLUDING THE STREAMS AN ENCODER WOULD NEVER PRODUCE.
//
// The two long ones came out of zlib and prove the ordinary path. The three
// short ones were assembled bit by bit, because the interesting cases are ones
// no encoder emits: a back-reference that reaches behind the start of the
// output is the buffer overrun this decoder exists to refuse, and there is no
// way to ask a compressor for one.
//
// THE OVERLAPPING COPY IS NOT A BUG AND HAS A TEST SO THAT NOBODY "FIXES" IT.
// A length of three at a distance of one means "repeat that byte three times",
// so the copy reads bytes it is in the middle of writing. Replacing that loop
// with memcpy is undefined behaviour and would break exactly this case, which
// is the commonest pattern in a run of flat colour — which is to say, in most
// of every PNG.
#include "../src/inflate.h"

#include <testing/test.h>

#include <string.h>

#include "inflate_data.inc"

static void a_back_reference_copies(void)
{
	uint8_t out[6] = { 0 };
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(voe_assets_inflate(BACKREF, sizeof(BACKREF), out,
					  sizeof(out), &error));
	VOE_TEST_CHECK(memcmp(out, "ABCABC", 6) == 0);
}

static void an_overlapping_run_repeats_a_byte(void)
{
	uint8_t out[4] = { 0 };
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(voe_assets_inflate(OVERLAP, sizeof(OVERLAP), out,
					  sizeof(out), &error));
	VOE_TEST_CHECK(memcmp(out, "AAAA", 4) == 0);
}

// THE ONE THAT MATTERS. One byte has been written and the stream asks to copy
// from four bytes before it, which is memory belonging to somebody else.
static void a_distance_past_the_start_is_refused(void)
{
	uint8_t out[4] = { 0 };
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(!voe_assets_inflate(TOOFAR, sizeof(TOOFAR), out,
					   sizeof(out), &error));
	VOE_TEST_CHECK_INT((int)error, (int)VOE_BASE_ERROR_MALFORMED);
}

static void a_compressed_stream_round_trips(void)
{
	uint8_t out[sizeof(PLAIN_EXPECTED)] = { 0 };
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(voe_assets_inflate(PLAIN_DEFLATED,
					  sizeof(PLAIN_DEFLATED), out,
					  sizeof(out), &error));
	VOE_TEST_CHECK(memcmp(out, PLAIN_EXPECTED, sizeof(out)) == 0);
}

static void a_stored_stream_round_trips(void)
{
	uint8_t out[sizeof(PLAIN_EXPECTED)] = { 0 };
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(voe_assets_inflate(PLAIN_STORED, sizeof(PLAIN_STORED),
					  out, sizeof(out), &error));
	VOE_TEST_CHECK(memcmp(out, PLAIN_EXPECTED, sizeof(out)) == 0);
}

// EXACTLY THE PROMISED SIZE, IN BOTH DIRECTIONS. Asking for one byte fewer than
// the stream produces is a picture whose header disagrees with its data, and
// asking for one more is a truncated stream. Both are the same lie and both are
// refused, which is what lets png.c size its buffer from the header and trust
// it.
static void the_capacity_must_match_exactly(void)
{
	uint8_t small[sizeof(PLAIN_EXPECTED) - 1] = { 0 };
	uint8_t large[sizeof(PLAIN_EXPECTED) + 1] = { 0 };
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(!voe_assets_inflate(PLAIN_DEFLATED,
					   sizeof(PLAIN_DEFLATED), small,
					   sizeof(small), &error));
	VOE_TEST_CHECK_INT((int)error, (int)VOE_BASE_ERROR_MALFORMED);

	VOE_TEST_CHECK(!voe_assets_inflate(PLAIN_DEFLATED,
					   sizeof(PLAIN_DEFLATED), large,
					   sizeof(large), &error));
	VOE_TEST_CHECK_INT((int)error, (int)VOE_BASE_ERROR_MALFORMED);
}

// Every prefix of a good stream, because a decompressor that runs off the end of
// its input is the other half of running off the end of its output.
//
// THE LAST FOUR BYTES ARE EXEMPT AND THAT IS NOT A GAP. They are zlib's
// Adler-32, which inflate.c deliberately does not check — by the time they are
// reached the DEFLATE data is complete and correct, so losing them changes
// nothing this function could detect and refusing them would mean checking a
// second checksum over bytes PNG has already put a CRC-32 on. A PNG that lost
// its tail still fails, one level up: the IDAT chunk's own CRC covers those four
// bytes and png.c checks every chunk. The sweep stops where the real stream
// does, so this tests truncation rather than testing that decision twice.
static void every_truncation_is_refused(void)
{
	for (size_t size = 0; size + 4 < sizeof(PLAIN_DEFLATED); size++) {
		uint8_t out[sizeof(PLAIN_EXPECTED)] = { 0 };
		voe_base_error error = VOE_BASE_OK;

		VOE_TEST_CHECK(!voe_assets_inflate(PLAIN_DEFLATED, size, out,
						   sizeof(out), &error));
	}
}

// The zlib wrapper, which is two bytes and three ways to be wrong.
static void a_bad_zlib_header_is_refused(void)
{
	uint8_t stream[sizeof(PLAIN_DEFLATED)];
	uint8_t out[sizeof(PLAIN_EXPECTED)];
	voe_base_error error = VOE_BASE_OK;

	// Compression method 7 is not DEFLATE.
	memcpy(stream, PLAIN_DEFLATED, sizeof(stream));
	stream[0] = (uint8_t)((stream[0] & 0xf0) | 7);
	VOE_TEST_CHECK(!voe_assets_inflate(stream, sizeof(stream), out,
					   sizeof(out), &error));

	// The header's own check value no longer divides by 31.
	memcpy(stream, PLAIN_DEFLATED, sizeof(stream));
	stream[1] ^= 0x01;
	VOE_TEST_CHECK(!voe_assets_inflate(stream, sizeof(stream), out,
					   sizeof(out), &error));

	// A preset dictionary this decoder does not have.
	memcpy(stream, PLAIN_DEFLATED, sizeof(stream));
	stream[1] |= 0x20;
	VOE_TEST_CHECK(!voe_assets_inflate(stream, sizeof(stream), out,
					   sizeof(out), &error));
}

int main(void)
{
	a_back_reference_copies();
	an_overlapping_run_repeats_a_byte();
	a_distance_past_the_start_is_refused();
	a_compressed_stream_round_trips();
	a_stored_stream_round_trips();
	the_capacity_must_match_exactly();
	every_truncation_is_refused();
	a_bad_zlib_header_is_refused();
	return voe_test_result();
}
