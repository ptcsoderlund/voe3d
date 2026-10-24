// voe_assets_image_fit, from outside: the sizes 0466 point 1 gives, a
// constant picture staying that constant for every kind, and each kind's own
// averaging on a picture small enough to work out by hand — colour in linear
// light and weighted by alpha, normals made unit length again, data as stored.
//
// The sizes are checked on pictures as large as the feature meets (4096 a
// side), pushed into the test's arena, so a footprint that walks off a row
// shows up as a wrong texel or a crash here rather than in an import.
#include <assets/image.h>

#include <testing/test.h>

#include <math.h>
#include <string.h>

// Big enough for the small pictures; the large ones get blocks of their own.
#define SCRATCH (1u << 20)

// A picture of width × height in `arena`, every texel `texel`.
static voe_assets_image filled(voe_base_arena *arena, uint32_t width,
			       uint32_t height, const uint8_t *texel)
{
	voe_assets_image image = { width, height, NULL, 0,
				   VOE_ASSETS_IMAGE_RGBA8 };

	image.pixels = voe_base_arena_push(arena, (size_t)width * height * 4);
	for (size_t i = 0; i < (size_t)width * height; i++)
		memcpy(image.pixels + i * 4, texel, 4);
	return image;
}

// A picture over pixels the test spells out texel by texel.
static voe_assets_image row_of(uint32_t width, uint32_t height,
			       uint8_t *pixels)
{
	return (voe_assets_image){ width, height, pixels, 0,
				   VOE_ASSETS_IMAGE_RGBA8 };
}

static void sizes(void)
{
	static const uint8_t GREY[4] = { 90, 90, 90, 255 };
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_assets_image wide = filled(arena, 4096, 2048, GREY);
	voe_assets_image tall = filled(arena, 1, 4096, GREY);
	voe_assets_image small = filled(arena, 300, 200, GREY);
	voe_assets_image odd = filled(arena, 3, 3, GREY);
	voe_assets_image fit;
	struct voe_base_arena_mark before;

	fit = voe_assets_image_fit(arena, wide, 1024, VOE_ASSETS_IMAGE_DATA);
	VOE_TEST_CHECK_INT((int)fit.width, 1024);
	VOE_TEST_CHECK_INT((int)fit.height, 512);
	VOE_TEST_CHECK_INT(fit.pixels[(1023 + 511 * 1024) * 4], 90);

	fit = voe_assets_image_fit(arena, tall, 512, VOE_ASSETS_IMAGE_DATA);
	VOE_TEST_CHECK_INT((int)fit.width, 1);
	VOE_TEST_CHECK_INT((int)fit.height, 512);

	// Not enlarged, not copied, and nothing pushed.
	before = voe_base_arena_mark(arena);
	fit = voe_assets_image_fit(arena, small, 512, VOE_ASSETS_IMAGE_COLOUR);
	VOE_TEST_CHECK(fit.pixels == small.pixels);
	VOE_TEST_CHECK_INT((int)fit.width, 300);
	VOE_TEST_CHECK_INT((int)fit.height, 200);
	VOE_TEST_CHECK(voe_base_arena_mark(arena).block == before.block);
	VOE_TEST_CHECK_INT((int)voe_base_arena_mark(arena).used,
			   (int)before.used);

	fit = voe_assets_image_fit(arena, odd, 2, VOE_ASSETS_IMAGE_DATA);
	VOE_TEST_CHECK_INT((int)fit.width, 2);
	VOE_TEST_CHECK_INT((int)fit.height, 2);

	voe_base_arena_destroy(arena);
}

// 3×3 at 2 puts a texel and a half under each footprint, so every output
// texel mixes whole and half-covered texels; the weights must still sum to 1.
static void a_constant_stays_constant(voe_assets_image_kind kind,
				      const uint8_t *texel)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_assets_image fit =
		voe_assets_image_fit(arena, filled(arena, 3, 3, texel), 2, kind);

	for (size_t i = 0; i < 2 * 2 * 4; i++)
		VOE_TEST_CHECK_INT(fit.pixels[i], texel[i % 4]);

	voe_base_arena_destroy(arena);
}

static void constants_for_each_kind(void)
{
	static const uint8_t COLOUR[4] = { 200, 100, 50, 180 };
	// (1, 0, 1) / √2, encoded.
	static const uint8_t NORMAL[4] = { 218, 128, 218, 90 };
	static const uint8_t DATA[4] = { 200, 100, 50, 180 };

	a_constant_stays_constant(VOE_ASSETS_IMAGE_COLOUR, COLOUR);
	a_constant_stays_constant(VOE_ASSETS_IMAGE_NORMAL, NORMAL);
	a_constant_stays_constant(VOE_ASSETS_IMAGE_DATA, DATA);
}

// Half black, half white is half the light: sRGB 188, where averaging the
// bytes would give 128.
static void colour_averages_in_linear_light(void)
{
	uint8_t pixels[] = { 0,	  0,   0,   255, 255, 255, 255, 255,
			     255, 255, 255, 255, 0,   0,   0,   255 };
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_assets_image fit = voe_assets_image_fit(
		arena, row_of(2, 2, pixels), 1, VOE_ASSETS_IMAGE_COLOUR);

	VOE_TEST_CHECK_INT((int)fit.width, 1);
	VOE_TEST_CHECK_INT((int)fit.height, 1);
	for (int c = 0; c < 3; c++)
		VOE_TEST_CHECK_FLOAT(fit.pixels[c], 188.0, 1.0);
	VOE_TEST_CHECK_INT(fit.pixels[3], 255);

	voe_base_arena_destroy(arena);
}

// A cut-out's hidden green does not bleed into its visible red.
static void colour_is_weighted_by_alpha(void)
{
	uint8_t pixels[] = { 255, 0, 0, 255, 0, 255, 0, 0 };
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_assets_image fit = voe_assets_image_fit(
		arena, row_of(2, 1, pixels), 1, VOE_ASSETS_IMAGE_COLOUR);

	VOE_TEST_CHECK_INT(fit.pixels[0], 255);
	VOE_TEST_CHECK_INT(fit.pixels[1], 0);
	VOE_TEST_CHECK_INT(fit.pixels[2], 0);
	VOE_TEST_CHECK_FLOAT(fit.pixels[3], 127.5, 0.5);

	voe_base_arena_destroy(arena);
}

static uint8_t encode_normal(float component)
{
	return (uint8_t)lroundf((component * 0.5f + 0.5f) * 255.0f);
}

static float decode_normal(uint8_t byte)
{
	return byte / 127.5f - 1.0f;
}

// Two normals leaning opposite ways average to straight out, at full length:
// averaged as colour they would come out shortened, a flatter bump.
static void normals_are_unit_length_again(void)
{
	const float lean = 1.0f / sqrtf(2.0f);
	uint8_t pixels[] = { encode_normal(lean),  encode_normal(0.0f),
			     encode_normal(lean),  255,
			     encode_normal(-lean), encode_normal(0.0f),
			     encode_normal(lean),  255 };
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_assets_image fit = voe_assets_image_fit(
		arena, row_of(2, 1, pixels), 1, VOE_ASSETS_IMAGE_NORMAL);
	const float x = decode_normal(fit.pixels[0]);
	const float y = decode_normal(fit.pixels[1]);
	const float z = decode_normal(fit.pixels[2]);
	const float step = 2.0f / 255.0f;

	VOE_TEST_CHECK_FLOAT(x, 0.0, step);
	VOE_TEST_CHECK_FLOAT(y, 0.0, step);
	VOE_TEST_CHECK_FLOAT(z, 1.0, step);
	VOE_TEST_CHECK_FLOAT(sqrtf(x * x + y * y + z * z), 1.0, step);
	VOE_TEST_CHECK_INT(fit.pixels[3], 255);

	voe_base_arena_destroy(arena);
}

// Data is averaged as stored, in every channel alpha included.
static void data_is_averaged_as_stored(void)
{
	uint8_t pixels[] = { 0, 0, 0, 0, 255, 255, 255, 255 };
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_assets_image fit = voe_assets_image_fit(
		arena, row_of(2, 1, pixels), 1, VOE_ASSETS_IMAGE_DATA);

	for (int c = 0; c < 4; c++)
		VOE_TEST_CHECK_FLOAT(fit.pixels[c], 127.5, 0.5);

	voe_base_arena_destroy(arena);
}

int main(void)
{
	sizes();
	constants_for_each_kind();
	colour_averages_in_linear_light();
	colour_is_weighted_by_alpha();
	normals_are_unit_length_again();
	data_is_averaged_as_stored();
	return voe_test_result();
}
