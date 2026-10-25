// A picture compressed to GPU blocks (assets/image.h): the format a kind
// takes, the block count of a chain, the compressed picture's room, and a run
// of its blocks encoded, each block's 64 bytes gathered from the source and
// handed to the per-block encoder of the format in image_blocks.h.
//
// CHAIN ORDER. Block n is found by walking the levels from the largest,
// subtracting each level's count, then row by row in that level; a run goes
// on from there, stepping to the next level as one runs out. Block n's 16
// bytes are at n · 16 in `pixels`, the layout image.h gives.
//
// AN EDGE BLOCK repeats the nearest edge texel where the level ends, so a
// 1×1 level is one colour sixteen times and its block is that colour, not a
// blend with zeros the GPU never samples.
//
// CONSTRAINTS. One thread, no allocation in the encode; the caller decides
// how many blocks a call does (0477 point 3).
#include <assets/image.h>

#include "image_blocks.h"

#include <base/assert.h>

#include <string.h>

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

// Block (bx, by) of a width × height RGBA8 level, an edge texel repeated
// where the level ends.
static void gather(const uint8_t *level, uint32_t width, uint32_t height,
		   uint32_t bx, uint32_t by, uint8_t rgba[64])
{
	for (uint32_t y = 0; y < 4; y++) {
		const uint32_t sy = by * 4 + y < height ? by * 4 + y : height - 1;

		for (uint32_t x = 0; x < 4; x++) {
			const uint32_t sx = bx * 4 + x < width ? bx * 4 + x :
								 width - 1;

			memcpy(rgba + (y * 4 + x) * 4,
			       level + ((size_t)sy * width + sx) * 4, 4);
		}
	}
}

void voe_assets_image_blocks_encode(const voe_assets_image *source,
				    voe_assets_image_kind kind,
				    voe_assets_image *target, uint32_t first,
				    uint32_t count)
{
	void (*const encode)(const uint8_t[64], uint8_t[16]) =
		voe_assets_image_blocks_format(kind) == VOE_ASSETS_IMAGE_BC5 ?
			voe_assets_bc5_encode :
			voe_assets_bc7_encode;
	const uint8_t *level = source->pixels;
	uint32_t i = 0, at = first, w = source->width, h = source->height;

	VOE_BASE_ASSERT(source->format == VOE_ASSETS_IMAGE_RGBA8 &&
				source->pixels != NULL,
			"blocks encoded from a picture with no RGBA8 texels");
	VOE_BASE_ASSERT(target->format == voe_assets_image_blocks_format(kind) &&
				target->width == source->width &&
				target->height == source->height &&
				held_levels(target) == held_levels(source),
			"blocks encoded into a picture of another shape");
	VOE_BASE_ASSERT(count <= voe_assets_image_block_count(target) &&
				first <= voe_assets_image_block_count(target) - count,
			"a run of blocks past the picture's count");
	for (uint32_t done = 0; done < count; done++) {
		uint8_t rgba[64];

		// The assert above keeps `at` inside the last level.
		while (i + 1 < held_levels(source) &&
		       at >= blocks_across(w) * blocks_across(h)) {
			at -= blocks_across(w) * blocks_across(h);
			level += (size_t)w * h * 4;
			i++;
			w = level_side(source->width, i);
			h = level_side(source->height, i);
		}
		gather(level, w, h, at % blocks_across(w), at / blocks_across(w),
		       rgba);
		encode(rgba, target->pixels + ((size_t)first + done) * 16);
		at++;
	}
}

voe_assets_image voe_assets_image_compress(voe_base_arena *arena,
					   const voe_assets_image *source,
					   voe_assets_image_kind kind)
{
	voe_assets_image out = voe_assets_image_blocks_new(arena, source, kind);

	voe_assets_image_blocks_encode(source, kind, &out, 0,
				       voe_assets_image_block_count(&out));
	return out;
}
