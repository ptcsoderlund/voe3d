// A picture compressed to blocks from outside: the sizes and block counts of
// 1×1, 5×3 and 256×128 chains; a smooth colour chain as BC7 and a gentle
// bump's normal map as BC5, every block decoded with the card's decoder near
// the texels it came from; runs of 7 blocks the same bytes as one call; and a
// 5×3 picture's edge blocks repeating its edge texels.
//
// The colour is a function of x + y alone, so every block's texels lie near
// one line in RGBA, the line BC7's mode 6 draws; the mips keep that, since a
// 2×2 average of x + y is again a function of the level's x + y.
#include <assets/image.h>

#include "../src/image_blocks.h"

#include <testing/test.h>

#include <math.h>
#include <string.h>

#define SCRATCH (1u << 22)
#define SIDE 64

static uint32_t level_side(uint32_t side, uint32_t level)
{
	return side >> level ? side >> level : 1;
}

// Every block of `blocks` decoded and each texel's first `channels` held
// within `tolerance` of `source`'s, an edge texel repeated past the level.
static void blocks_decode_near(const voe_assets_image *source,
			       const voe_assets_image *blocks, int channels,
			       double tolerance)
{
	const uint8_t *level = source->pixels;
	const uint8_t *block = blocks->pixels;
	const uint32_t levels = source->levels > 1 ? source->levels : 1;

	for (uint32_t i = 0; i < levels; i++) {
		const uint32_t w = level_side(source->width, i);
		const uint32_t h = level_side(source->height, i);

		for (uint32_t by = 0; by < (h + 3) / 4; by++)
			for (uint32_t bx = 0; bx < (w + 3) / 4; bx++, block += 16) {
				uint8_t decoded[64];

				if (blocks->format == VOE_ASSETS_IMAGE_BC5)
					voe_assets_bc5_decode(block, decoded);
				else
					voe_assets_bc7_decode(block, decoded);
				for (uint32_t t = 0; t < 16; t++) {
					const uint32_t x = bx * 4 + t % 4;
					const uint32_t y = by * 4 + t / 4;
					const uint8_t *texel =
						level + ((size_t)(y < h ? y : h - 1) * w +
							 (x < w ? x : w - 1)) * 4;

					for (int c = 0; c < channels; c++)
						VOE_TEST_CHECK_FLOAT(decoded[t * 4 + c],
								     texel[c], tolerance);
				}
			}
		level += (size_t)w * h * 4;
	}
	VOE_TEST_CHECK(block == blocks->pixels + voe_assets_image_size(blocks));
}

// The picture with a chain of `levels`, room for the blocks, and the counts.
static void counts(voe_base_arena *arena, uint32_t width, uint32_t height,
		   uint32_t levels, uint32_t block_count)
{
	const voe_assets_image source = { .width = width,
					   .height = height,
					   .levels = levels };
	const voe_assets_image blocks =
		voe_assets_image_blocks_new(arena, &source, VOE_ASSETS_IMAGE_DATA);

	VOE_TEST_CHECK_INT((int)voe_assets_image_block_count(&blocks),
			   (int)block_count);
	VOE_TEST_CHECK_INT((int)voe_assets_image_size(&blocks),
			   (int)block_count * 16);
	VOE_TEST_CHECK_INT((int)blocks.levels, (int)levels);
	VOE_TEST_CHECK_INT(blocks.format, VOE_ASSETS_IMAGE_BC7);
	for (size_t i = 0; i < voe_assets_image_size(&blocks); i++)
		VOE_TEST_CHECK_INT(blocks.pixels[i], 0);
}

static void sizes_and_block_counts(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	const voe_assets_image rgba = { .width = 5, .height = 3, .levels = 3 };

	counts(arena, 1, 1, 1, 1);
	// 2×1, 1×1 and 1×1 blocks.
	counts(arena, 5, 3, 3, 4);
	// 2048 + 512 + 128 + 32 + 8 + 2 + 1 + 1 + 1.
	counts(arena, 256, 128, 9, 2733);
	VOE_TEST_CHECK_INT((int)voe_assets_image_size(&rgba), (15 + 2 + 1) * 4);
	VOE_TEST_CHECK_INT(voe_assets_image_blocks_format(VOE_ASSETS_IMAGE_NORMAL),
			   VOE_ASSETS_IMAGE_BC5);
	VOE_TEST_CHECK_INT(voe_assets_image_blocks_format(VOE_ASSETS_IMAGE_COLOUR),
			   VOE_ASSETS_IMAGE_BC7);

	voe_base_arena_destroy(arena);
}

// A 64×64 colour picture smooth along x + y, with its chain.
static voe_assets_image colour_chain(voe_base_arena *arena)
{
	voe_assets_image one = { .width = SIDE, .height = SIDE };

	one.pixels = voe_base_arena_push(arena, SIDE * SIDE * 4);
	for (uint32_t t = 0; t < SIDE * SIDE; t++) {
		const uint32_t s = t % SIDE + t / SIDE;

		one.pixels[t * 4 + 0] = (uint8_t)(30 + s);
		one.pixels[t * 4 + 1] = (uint8_t)(200 - s);
		one.pixels[t * 4 + 2] = (uint8_t)(60 + s / 2);
		one.pixels[t * 4 + 3] = 255;
	}
	return voe_assets_image_mips(arena, &one, VOE_ASSETS_IMAGE_COLOUR);
}

static void a_colour_chain_decodes_within_4(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	const voe_assets_image chain = colour_chain(arena);
	const voe_assets_image blocks =
		voe_assets_image_compress(arena, &chain, VOE_ASSETS_IMAGE_COLOUR);

	VOE_TEST_CHECK_INT(blocks.format, VOE_ASSETS_IMAGE_BC7);
	VOE_TEST_CHECK_INT((int)blocks.levels, 7);
	blocks_decode_near(&chain, &blocks, 4, 4.0);

	voe_base_arena_destroy(arena);
}

// x and y swing ±36 over 64 texels, so neither moves more than 4 a texel.
static void a_bump_map_decodes_within_3(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_assets_image bump = { .width = SIDE, .height = SIDE };
	const float turn = 6.2831853f / SIDE;

	bump.pixels = voe_base_arena_push(arena, SIDE * SIDE * 4);
	for (uint32_t t = 0; t < SIDE * SIDE; t++) {
		bump.pixels[t * 4 + 0] =
			(uint8_t)(128 + lroundf(36 * sinf(turn * (t % SIDE))));
		bump.pixels[t * 4 + 1] =
			(uint8_t)(128 + lroundf(36 * sinf(turn * (t / SIDE))));
		bump.pixels[t * 4 + 2] = 230;
		bump.pixels[t * 4 + 3] = 255;
	}
	const voe_assets_image blocks =
		voe_assets_image_compress(arena, &bump, VOE_ASSETS_IMAGE_NORMAL);

	VOE_TEST_CHECK_INT(blocks.format, VOE_ASSETS_IMAGE_BC5);
	blocks_decode_near(&bump, &blocks, 2, 3.0);

	voe_base_arena_destroy(arena);
}

static void runs_of_7_match_one_call(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	const voe_assets_image chain = colour_chain(arena);
	const voe_assets_image whole =
		voe_assets_image_compress(arena, &chain, VOE_ASSETS_IMAGE_COLOUR);
	voe_assets_image runs =
		voe_assets_image_blocks_new(arena, &chain, VOE_ASSETS_IMAGE_COLOUR);
	const uint32_t total = voe_assets_image_block_count(&runs);

	for (uint32_t first = 0; first < total; first += 7)
		voe_assets_image_blocks_encode(&chain, VOE_ASSETS_IMAGE_COLOUR,
					       &runs, first,
					       total - first < 7 ? total - first : 7);
	VOE_TEST_CHECK(memcmp(runs.pixels, whole.pixels,
			      voe_assets_image_size(&whole)) == 0);

	voe_base_arena_destroy(arena);
}

// Greys rising 5 a column and 20 a row: zeros where the picture ends would
// put a decoded texel 100 away.
static void edge_blocks_repeat_the_edge(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_assets_image grey = { .width = 5, .height = 3 };

	grey.pixels = voe_base_arena_push(arena, 5 * 3 * 4);
	for (uint32_t t = 0; t < 15; t++) {
		memset(grey.pixels + t * 4, 100 + 5 * (t % 5) + 20 * (t / 5), 3);
		grey.pixels[t * 4 + 3] = 255;
	}
	const voe_assets_image blocks =
		voe_assets_image_compress(arena, &grey, VOE_ASSETS_IMAGE_DATA);

	VOE_TEST_CHECK_INT((int)voe_assets_image_block_count(&blocks), 2);
	blocks_decode_near(&grey, &blocks, 4, 3.0);

	voe_base_arena_destroy(arena);
}

int main(void)
{
	sizes_and_block_counts();
	a_colour_chain_decodes_within_4();
	a_bump_map_decodes_within_3();
	runs_of_7_match_one_call();
	edge_blocks_repeat_the_edge();
	return voe_test_result();
}
