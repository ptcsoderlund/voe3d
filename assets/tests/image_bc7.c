// BC7 mode 6 from outside: a block built bit by bit decoded to the texels the
// format defines, and blocks the encoder wrote decoded back near what went
// in: one colour within 1 a channel, two colours within 3, a smooth gradient
// within 4, a cut-out's alpha 0 and 255 exact, and the same texels encoded
// twice to the same bytes.
//
// The hand-built block's bytes and texels were worked out from the format,
// not from this encoder: mode 6, endpoints (17, 201, 1, 255) with p-bit 1 and
// (240, 40, 254, 128) with p-bit 0, and texel t at index t, so every weight
// from 0 to 64 is read once.
#include "../src/image_blocks.h"

#include <testing/test.h>

#include <string.h>

static void a_hand_built_block_decodes_to_its_texels(void)
{
	static const uint8_t BLOCK[16] = { 0x40, 0x04, 0x9e, 0x4c, 0x01, 0xfc,
					   0xff, 0xc0, 0x10, 0x32, 0x54, 0x76,
					   0x98, 0xba, 0xdc, 0xfe };
	static const uint8_t TEXELS[64] = {
		17,  201, 1,   255, 31,  191, 17,  247, 48,  178, 37,  237,
		62,  168, 52,  229, 76,  158, 68,  221, 90,  148, 84,  213,
		108, 136, 104, 203, 122, 126, 120, 195, 135, 115, 135, 188,
		149, 105, 151, 180, 167, 93,  171, 170, 181, 83,  187, 162,
		195, 73,  203, 154, 209, 63,  218, 146, 226, 50,  238, 136,
		240, 40,  254, 128,
	};
	uint8_t rgba[64];

	voe_assets_bc7_decode(BLOCK, rgba);
	for (int i = 0; i < 64; i++)
		VOE_TEST_CHECK_INT(rgba[i], TEXELS[i]);
}

// `rgba` encoded and decoded, every channel of every texel within `tolerance`.
static void round_trips_within(const uint8_t rgba[64], double tolerance)
{
	uint8_t block[16], decoded[64];

	voe_assets_bc7_encode(rgba, block);
	voe_assets_bc7_decode(block, decoded);
	for (int i = 0; i < 64; i++)
		VOE_TEST_CHECK_FLOAT(decoded[i], rgba[i], tolerance);
}

static void one_colour_round_trips_within_1(void)
{
	static const uint8_t COLOURS[3][4] = {
		{ 201, 100, 50, 255 },
		{ 13, 254, 128, 77 },
		{ 0, 0, 0, 0 },
	};

	for (int k = 0; k < 3; k++) {
		uint8_t rgba[64];

		for (int t = 0; t < 16; t++)
			memcpy(rgba + t * 4, COLOURS[k], 4);
		round_trips_within(rgba, 1.0);
	}
}

static void two_colours_round_trip_within_3(void)
{
	static const uint8_t COLOURS[2][4] = {
		{ 30, 200, 91, 255 },
		{ 221, 40, 160, 128 },
	};
	uint8_t rgba[64];

	for (int t = 0; t < 16; t++)
		memcpy(rgba + t * 4, COLOURS[(t % 4 + t / 4) % 2], 4);
	round_trips_within(rgba, 3.0);
}

// Diagonal steps of 8, 6 and 4 in red, green and blue, opaque.
static void a_smooth_gradient_round_trips_within_4(void)
{
	uint8_t rgba[64];

	for (int t = 0; t < 16; t++) {
		const int s = t % 4 + t / 4;

		rgba[t * 4 + 0] = (uint8_t)(20 + 8 * s);
		rgba[t * 4 + 1] = (uint8_t)(200 - 6 * s);
		rgba[t * 4 + 2] = (uint8_t)(90 + 4 * s);
		rgba[t * 4 + 3] = 255;
	}
	round_trips_within(rgba, 4.0);
}

// A cut-out: the corners transparent, the rest opaque, colour odd and even
// so no single p-bit suits it.
static void alpha_0_and_255_stay_exact(void)
{
	static const uint8_t CLEAR[4] = { 37, 90, 143, 0 };
	static const uint8_t SOLID[4] = { 200, 151, 100, 255 };
	uint8_t rgba[64], block[16], decoded[64];

	for (int t = 0; t < 16; t++) {
		const bool corner = (t == 0 || t == 3 || t == 12 || t == 15);

		memcpy(rgba + t * 4, corner ? CLEAR : SOLID, 4);
	}
	voe_assets_bc7_encode(rgba, block);
	voe_assets_bc7_decode(block, decoded);
	for (int t = 0; t < 16; t++) {
		VOE_TEST_CHECK_INT(decoded[t * 4 + 3], rgba[t * 4 + 3]);
		for (int c = 0; c < 3; c++)
			VOE_TEST_CHECK_FLOAT(decoded[t * 4 + c], rgba[t * 4 + c],
					     1.0);
	}
}

// The second block starts full of ones, so a bit left unwritten shows.
static void the_same_texels_give_the_same_bytes(void)
{
	uint8_t rgba[64], first[16] = { 0 }, second[16];

	memset(second, 0xff, sizeof second);
	for (int i = 0; i < 64; i++)
		rgba[i] = (uint8_t)(i * 37 + 11);
	voe_assets_bc7_encode(rgba, first);
	voe_assets_bc7_encode(rgba, second);
	VOE_TEST_CHECK(memcmp(first, second, sizeof first) == 0);
}

int main(void)
{
	a_hand_built_block_decodes_to_its_texels();
	one_colour_round_trips_within_1();
	two_colours_round_trip_within_3();
	a_smooth_gradient_round_trips_within_4();
	alpha_0_and_255_stay_exact();
	the_same_texels_give_the_same_bytes();
	return voe_test_result();
}
