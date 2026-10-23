// A picture's levels from outside: the chain's length as Vulkan counts it, a
// constant picture's mips staying that constant for every kind, a checker
// halving to the grey of linear light, and the cooked form round-tripped,
// refused when its magic is wrong or its bytes cut short, and its key read
// from the head alone.
//
// 5×3 is the shape that matters: its sides halve to 2×1 and then 1×1, so one
// side bottoms out at 1 before the other and every footprint is uneven.
#include <assets/image.h>

#include <testing/test.h>

#include <string.h>

#define SCRATCH (1u << 20)
#define KEY 0x0123456789abcdefull

// A width × height picture in `arena`, every texel `texel`.
static voe_assets_image filled(voe_base_arena *arena, uint32_t width,
			       uint32_t height, const uint8_t *texel)
{
	voe_assets_image image = { .width = width, .height = height };

	image.pixels = voe_base_arena_push(arena, (size_t)width * height * 4);
	for (size_t i = 0; i < (size_t)width * height; i++)
		memcpy(image.pixels + i * 4, texel, 4);
	return image;
}

static void level_count_follows_vulkan(void)
{
	VOE_TEST_CHECK_INT((int)voe_assets_image_level_count(1, 1), 1);
	VOE_TEST_CHECK_INT((int)voe_assets_image_level_count(5, 3), 3);
	VOE_TEST_CHECK_INT((int)voe_assets_image_level_count(256, 1), 9);
}

static void a_constant_stays_constant(voe_assets_image_kind kind,
				      const uint8_t *texel)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	const voe_assets_image one = filled(arena, 5, 3, texel);
	const voe_assets_image mips = voe_assets_image_mips(arena, &one, kind);
	const size_t size = voe_assets_image_size(&mips);

	VOE_TEST_CHECK_INT((int)mips.levels, 3);
	// 5×3, 2×1 and 1×1.
	VOE_TEST_CHECK_INT((int)size, (15 + 2 + 1) * 4);
	VOE_TEST_CHECK_INT((int)voe_assets_image_size(&one), 15 * 4);
	for (size_t i = 0; i < size; i++)
		VOE_TEST_CHECK_INT(mips.pixels[i], texel[i % 4]);

	voe_base_arena_destroy(arena);
}

static void mips_of_a_constant_stay_constant(void)
{
	static const uint8_t COLOUR[4] = { 200, 100, 50, 180 };
	// (1, 0, 1) / √2, encoded.
	static const uint8_t NORMAL[4] = { 218, 128, 218, 90 };
	static const uint8_t DATA[4] = { 7, 100, 250, 0 };

	a_constant_stays_constant(VOE_ASSETS_IMAGE_COLOUR, COLOUR);
	a_constant_stays_constant(VOE_ASSETS_IMAGE_NORMAL, NORMAL);
	a_constant_stays_constant(VOE_ASSETS_IMAGE_DATA, DATA);
}

// A one-texel black and white checker is half the light at every level
// below the first: sRGB 188, where averaging the bytes would give 128.
static void mips_halve_a_checker_to_grey(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_assets_image checker = { .width = 4, .height = 4 };
	voe_assets_image mips;

	checker.pixels = voe_base_arena_push(arena, 4 * 4 * 4);
	for (uint32_t i = 0; i < 16; i++) {
		const uint8_t value = ((i % 4) + (i / 4)) % 2 ? 255 : 0;

		memset(checker.pixels + i * 4, value, 3);
		checker.pixels[i * 4 + 3] = 255;
	}
	mips = voe_assets_image_mips(arena, &checker, VOE_ASSETS_IMAGE_COLOUR);

	VOE_TEST_CHECK_INT((int)mips.levels, 3);
	VOE_TEST_CHECK(memcmp(mips.pixels, checker.pixels, 64) == 0);
	// Levels 1 and 2: 2×2 then 1×1, five texels after level 0.
	for (size_t t = 16; t < 21; t++) {
		for (int c = 0; c < 3; c++)
			VOE_TEST_CHECK_FLOAT(mips.pixels[t * 4 + c], 188.0, 1.0);
		VOE_TEST_CHECK_INT(mips.pixels[t * 4 + 3], 255);
	}

	voe_base_arena_destroy(arena);
}

// A mipped 5×3 data picture, cooked.
static voe_assets_bytes cooked(voe_base_arena *arena,
			       voe_assets_image *mips)
{
	static const uint8_t TEXEL[4] = { 1, 2, 3, 4 };
	const voe_assets_image one = filled(arena, 5, 3, TEXEL);

	*mips = voe_assets_image_mips(arena, &one, VOE_ASSETS_IMAGE_DATA);
	for (size_t i = 0; i < voe_assets_image_size(mips); i++)
		mips->pixels[i] = (uint8_t)(i * 7);
	return voe_assets_image_cooked_write(arena, mips,
					     VOE_ASSETS_IMAGE_NORMAL, KEY);
}

static void cooked_round_trips(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_assets_image mips;
	const voe_assets_bytes bytes = cooked(arena, &mips);
	voe_assets_image read = { 0 };
	voe_assets_image_kind kind = VOE_ASSETS_IMAGE_DATA;
	uint64_t key = 0;
	size_t used = 0;

	VOE_TEST_CHECK_INT((int)(bytes.count % 16), 0);
	VOE_TEST_CHECK_INT((int)bytes.count,
			   (int)(VOE_ASSETS_IMAGE_COOKED_HEAD + 80));
	VOE_TEST_CHECK(voe_assets_image_cooked_read(bytes.bytes, bytes.count,
						    &read, &kind, &key, &used));
	VOE_TEST_CHECK_INT((int)kind, VOE_ASSETS_IMAGE_NORMAL);
	VOE_TEST_CHECK(key == KEY);
	VOE_TEST_CHECK_INT((int)used, (int)bytes.count);
	VOE_TEST_CHECK_INT((int)read.width, 5);
	VOE_TEST_CHECK_INT((int)read.height, 3);
	VOE_TEST_CHECK_INT((int)read.levels, 3);
	VOE_TEST_CHECK(read.pixels == bytes.bytes + VOE_ASSETS_IMAGE_COOKED_HEAD);
	VOE_TEST_CHECK(memcmp(read.pixels, mips.pixels,
			      voe_assets_image_size(&mips)) == 0);

	voe_base_arena_destroy(arena);
}

static void cooked_refuses_a_wrong_magic_or_short_bytes(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_assets_image mips;
	const voe_assets_bytes bytes = cooked(arena, &mips);
	voe_assets_image read = { 0 };
	voe_assets_image_kind kind = VOE_ASSETS_IMAGE_COLOUR;
	uint64_t key = 0;
	size_t used = 0;

	VOE_TEST_CHECK(!voe_assets_image_cooked_read(
		bytes.bytes, bytes.count - 1, &read, &kind, &key, &used));
	VOE_TEST_CHECK(!voe_assets_image_cooked_read(
		bytes.bytes, VOE_ASSETS_IMAGE_COOKED_HEAD - 1, &read, &kind,
		&key, &used));
	bytes.bytes[0] ^= 0xff;
	VOE_TEST_CHECK(!voe_assets_image_cooked_read(bytes.bytes, bytes.count,
						     &read, &kind, &key, &used));
	VOE_TEST_CHECK(!voe_assets_image_cooked_key(bytes.bytes, bytes.count,
						    &key));
	// Nothing written on a refusal.
	VOE_TEST_CHECK(read.pixels == NULL);
	VOE_TEST_CHECK_INT((int)kind, VOE_ASSETS_IMAGE_COLOUR);
	VOE_TEST_CHECK(key == 0);
	VOE_TEST_CHECK_INT((int)used, 0);

	voe_base_arena_destroy(arena);
}

static void key_reads_from_the_head_alone(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_assets_image mips;
	const voe_assets_bytes bytes = cooked(arena, &mips);
	uint64_t key = 0;

	VOE_TEST_CHECK(voe_assets_image_cooked_key(
		bytes.bytes, VOE_ASSETS_IMAGE_COOKED_HEAD, &key));
	VOE_TEST_CHECK(key == KEY);
	VOE_TEST_CHECK(!voe_assets_image_cooked_key(
		bytes.bytes, VOE_ASSETS_IMAGE_COOKED_HEAD - 1, &key));

	voe_base_arena_destroy(arena);
}

int main(void)
{
	level_count_follows_vulkan();
	mips_of_a_constant_stay_constant();
	mips_halve_a_checker_to_grey();
	cooked_round_trips();
	cooked_refuses_a_wrong_magic_or_short_bytes();
	key_reads_from_the_head_alone();
	return voe_test_result();
}
