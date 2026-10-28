// A picture's GPU blocks, their shape and not their making (assets/image.h):
// the format a kind takes, the block count of a chain, and the compressed
// picture's room. render's encoder fills that room on the GPU (0484, 0486).
//
// A level of w × h holds ⌈w/4⌉ × ⌈h/4⌉ blocks of 16 bytes, the levels back to
// back largest first, the layout image.h gives.
//
// CONSTRAINTS. No allocation but the room's, pushed to the caller's arena.
#include <assets/image.h>

#include <base/assert.h>

static uint32_t level_side(uint32_t side, uint32_t level)
{
	return side >> level ? side >> level : 1;
}

static uint32_t blocks_across(uint32_t side)
{
	return (side + 3) / 4;
}

static uint32_t held_levels(const voe_assets_image *image)
{
	return image->levels > 1 ? image->levels : 1;
}

voe_assets_image_format voe_assets_image_blocks_format(voe_assets_image_kind kind)
{
	VOE_BASE_ASSERT(kind <= VOE_ASSETS_IMAGE_DATA, "a picture of no kind");
	return kind == VOE_ASSETS_IMAGE_NORMAL ? VOE_ASSETS_IMAGE_BC5 :
						 VOE_ASSETS_IMAGE_BC7;
}

uint32_t voe_assets_image_block_count(const voe_assets_image *image)
{
	uint32_t count = 0;

	VOE_BASE_ASSERT(image->width >= 1 && image->height >= 1,
			"the blocks of a picture with no size");
	for (uint32_t i = 0; i < held_levels(image); i++)
		count += blocks_across(level_side(image->width, i)) *
			 blocks_across(level_side(image->height, i));
	return count;
}

voe_assets_image voe_assets_image_blocks_new(voe_base_arena *arena,
					     const voe_assets_image *source,
					     voe_assets_image_kind kind)
{
	voe_assets_image out = {
		.width = source->width,
		.height = source->height,
		.levels = source->levels,
		.format = voe_assets_image_blocks_format(kind),
	};

	VOE_BASE_ASSERT(source->format == VOE_ASSETS_IMAGE_RGBA8,
			"a picture already compressed compressed again");
	out.pixels = voe_base_arena_push(arena, voe_assets_image_size(&out));
	return out;
}
