// THE JPEG READER, AGAINST FILES BUILT BY AN ENCODER WRITTEN FOR THE PURPOSE.
//
// There is no JPEG in this repository to test against and no library here to
// make one, so the files below came out of a small baseline encoder written
// alongside this test: standard Huffman tables from the specification's annex,
// a quantisation table of all ones, and the same forward transform the decoder
// inverts. That is worth stating plainly, because a decoder tested only against
// its own author's encoder can agree with it about something they are both
// wrong about.
//
// WHAT KEEPS THAT HONEST IS THE QUANTISATION TABLE OF ONES. It makes the round
// trip very nearly lossless, so these tests compare against the pixels that went
// in rather than against whatever came out the first time it was run. A
// transform with a sign error, an off-by-one in the zigzag or a wrong colour
// matrix cannot survive that comparison, whatever produced the file — and none
// of those mistakes is one an encoder and a decoder written together would
// cancel out, because the encoder's tables are the specification's and not this
// engine's.
//
// TOLERANCE IS THREE LEVELS OUT OF 255, WHICH IS THE COSINE TRANSFORM'S OWN
// ROUNDING AND NOT SLACK FOR BUGS. Even at a quantiser of one, the coefficients
// are integers and the two transforms are floating point, so a sample can move
// by a level or two. Three is comfortably inside that and nowhere near enough to
// hide a wrong pixel: every mistake this test is looking for moves samples by
// tens or by hundreds.
#include <assets/image.h>

#include <testing/test.h>

#include <string.h>

#include "jpeg_data.inc"

#define SCRATCH (1u << 20)
#define TOLERANCE 3

static void close_enough(int got, int want)
{
	VOE_TEST_CHECK(got >= want - TOLERANCE && got <= want + TOLERANCE);
}

[[nodiscard]] static voe_base_arena *decodes_to(const uint8_t *bytes,
						size_t size, uint32_t width,
						uint32_t height,
						voe_assets_image *image)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(voe_assets_jpeg_decode(bytes, size, arena, image,
					      &error));
	VOE_TEST_CHECK_INT((int)error, (int)VOE_BASE_OK);
	VOE_TEST_CHECK_INT((int)image->width, (int)width);
	VOE_TEST_CHECK_INT((int)image->height, (int)height);
	return arena;
}

static void refuses(const uint8_t *bytes, size_t size, voe_base_error expected)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_assets_image image = { 0 };
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(!voe_assets_jpeg_decode(bytes, size, arena, &image,
					       &error));
	VOE_TEST_CHECK_INT((int)error, (int)expected);
	voe_base_arena_destroy(arena);
}

// One component, so no colour conversion and no upsampling: this is the
// Huffman decode, the dequantise and the inverse transform on their own. If this
// fails, nothing below is worth reading.
static void greyscale_round_trips(void)
{
	voe_assets_image image = { 0 };
	voe_base_arena *arena;

	arena = decodes_to(JPEG_GREY, sizeof(JPEG_GREY), 16, 16, &image);

	for (uint32_t i = 0; i < 16 * 16; i++) {
		close_enough(image.pixels[i * 4 + 0], GREY_EXPECTED[i]);
		close_enough(image.pixels[i * 4 + 1], GREY_EXPECTED[i]);
		close_enough(image.pixels[i * 4 + 2], GREY_EXPECTED[i]);
		VOE_TEST_CHECK_INT(image.pixels[i * 4 + 3], 0xff);
	}

	voe_base_arena_destroy(arena);
}

// The colours the picture was built from. The left half is red-ish and the top
// half green-ish, so the four quadrants are four different colours and a wrong
// colour matrix or a swapped Cb and Cr cannot pass.
static void check_quadrants(const voe_assets_image *image)
{
	const uint8_t *top_left = image->pixels + (4 * 16 + 4) * 4;
	const uint8_t *top_right = image->pixels + (4 * 16 + 12) * 4;
	const uint8_t *bottom_left = image->pixels + (12 * 16 + 4) * 4;
	const uint8_t *bottom_right = image->pixels + (12 * 16 + 12) * 4;

	// Left half is bright red, right half dark. Top half bright green,
	// bottom half dark. So the top-left is yellow-white and the
	// bottom-right is nearly black.
	VOE_TEST_CHECK(top_left[0] > 200);
	VOE_TEST_CHECK(top_left[1] > 200);
	VOE_TEST_CHECK(top_right[0] < 90);
	VOE_TEST_CHECK(top_right[1] > 200);
	VOE_TEST_CHECK(bottom_left[0] > 200);
	VOE_TEST_CHECK(bottom_left[1] < 90);
	VOE_TEST_CHECK(bottom_right[0] < 90);
	VOE_TEST_CHECK(bottom_right[1] < 90);

	// Every pixel is opaque; JPEG has no alpha to get wrong.
	for (uint32_t i = 0; i < 16 * 16; i++)
		VOE_TEST_CHECK_INT(image->pixels[i * 4 + 3], 0xff);
}

// Three components at full resolution: the colour conversion, with the
// upsampler doing nothing.
static void colour_444_decodes(void)
{
	voe_assets_image image = { 0 };
	voe_base_arena *arena;

	arena = decodes_to(JPEG_444, sizeof(JPEG_444), 16, 16, &image);
	check_quadrants(&image);
	voe_base_arena_destroy(arena);
}

// THE SUBSAMPLED CASE, WHICH IS WHAT ALMOST EVERY JPEG IN THE WORLD ACTUALLY IS.
// Luma at full size and chroma at half in both directions, so one MCU is four
// luma blocks and one of each chroma — a different path through the block loop,
// and the one where a decoder that assumed one block per component per MCU falls
// apart. The colours must still land in the right quadrants.
static void colour_420_decodes(void)
{
	voe_assets_image image = { 0 };
	voe_base_arena *arena;

	arena = decodes_to(JPEG_420, sizeof(JPEG_420), 16, 16, &image);
	check_quadrants(&image);
	voe_base_arena_destroy(arena);
}

// WELL-FORMED AND DECLINED, NOT BROKEN. A progressive file is one a great many
// programs read perfectly well; this one does not, and it says so with a
// different code from the one it uses for damage. Decoding it with the baseline
// scan loop would produce a corrupt picture, which is the outcome worth
// preventing.
static void progressive_is_unsupported(void)
{
	refuses(JPEG_PROGRESSIVE, sizeof(JPEG_PROGRESSIVE),
		VOE_BASE_ERROR_UNSUPPORTED);
}

// Sixty-five thousand square is the largest a JPEG header can even ask for, and
// it is four thousand times more picture than this engine will decode. Refused
// on the header, before anything is allocated.
static void an_absurd_declared_size_is_refused(void)
{
	refuses(JPEG_HUGE, sizeof(JPEG_HUGE), VOE_BASE_ERROR_MALFORMED);
}

static void a_wrong_magic_number_is_refused(void)
{
	uint8_t bytes[sizeof(JPEG_GREY)];
	static const uint8_t PNG_START[8] = { 0x89, 'P', 'N', 'G',
					      0x0d, 0x0a, 0x1a, 0x0a };

	memcpy(bytes, JPEG_GREY, sizeof(bytes));
	bytes[1] = 0xd9;
	refuses(bytes, sizeof(bytes), VOE_BASE_ERROR_MALFORMED);

	// A PNG handed to the JPEG reader, which is the mistake a caller that
	// guesses the format from a file name actually makes.
	refuses(PNG_START, sizeof(PNG_START), VOE_BASE_ERROR_MALFORMED);
}

// EVERY PREFIX, AND THE POINT IS THAT NONE OF THEM CRASHES. A truncated JPEG is
// the case where a decoder walks off the end of a marker payload, a Huffman
// table or the coded data, and the check that it *failed* matters less than the
// fact that it came back at all. Running this under a sanitiser is what makes it
// worth having.
static void every_truncation_is_refused(void)
{
	for (size_t size = 0; size < sizeof(JPEG_GREY); size++)
		refuses(JPEG_GREY, size, VOE_BASE_ERROR_MALFORMED);
}

int main(void)
{
	greyscale_round_trips();
	colour_444_decodes();
	colour_420_decodes();
	progressive_is_unsupported();
	an_absurd_declared_size_is_refused();
	a_wrong_magic_number_is_refused();
	every_truncation_is_refused();
	return voe_test_result();
}
