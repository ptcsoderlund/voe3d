// A picture's GPU blocks from outside: the sizes and block counts of 1×1,
// 5×3 and 256×128 chains, the room _blocks_new makes zeroed, and the format
// each kind takes. The blocks themselves are render's encoder's, tested there.
#include <assets/image.h>

#include <testing/test.h>

#define SCRATCH (1u << 22)

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
	VOE_TEST_CHECK_INT(voe_assets_image_blocks_format(VOE_ASSETS_IMAGE_DATA),
			   VOE_ASSETS_IMAGE_BC7);

	voe_base_arena_destroy(arena);
}

int main(void)
{
	sizes_and_block_counts();
	return voe_test_result();
}
