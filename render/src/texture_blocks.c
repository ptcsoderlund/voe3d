// A SMOOTH texture uploaded as BC7 or BC5 blocks, its chain given:
// voe_render_texture_create_blocks, declared in render/include/render/device.h.
//
// THE BLOCKS ARE COPIED AS THEY ARE (ADR-0477). The encoder in `assets` made them
// on the CPU and the card decodes them on every read, so this file only picks the
// format and names where each level sits: BC7 SRGB for a colour map, BC7 UNORM
// for a data map, BC5 UNORM for a normal map of either kind.
//
// ONE REGION PER LEVEL, AS texture_given.c HAS. A level is ⌈w/4⌉ × ⌈h/4⌉ blocks of
// 16 bytes, the levels back to back, and a buffer row length of 0 reads them
// packed. Each region's extent is the level's own size, not rounded up to blocks:
// a copy into a compressed image takes an extent of whole blocks or one that
// reaches the level's edge, and a 2×2 or 1×1 level is only the second.
// texture.c's upload records the one copy, moves every level to the shader and
// idles, as it does for _create_levels.
//
// Constraints: at most 32 levels, the full chain of a 32-bit side, so the regions
// are on the stack. Without textureCompressionBC on the device every call fails
// REFUSED with one line before a slot is touched.
#include "device_internal.h"

#include <base/assert.h>
#include <base/report.h>

static VkFormat blocks_format(voe_render_texture_kind kind,
			      voe_render_blocks blocks)
{
	VOE_BASE_DEBUG_ASSERT(blocks == VOE_RENDER_BLOCKS_BC7 ||
				      blocks == VOE_RENDER_BLOCKS_BC5,
			      "a block format this file does not know");

	if (blocks == VOE_RENDER_BLOCKS_BC5)
		return VK_FORMAT_BC5_UNORM_BLOCK;
	return kind == VOE_RENDER_TEXTURE_COLOUR ? VK_FORMAT_BC7_SRGB_BLOCK
						 : VK_FORMAT_BC7_UNORM_BLOCK;
}

static bool create_blocks(voe_render_device *device,
			  voe_render_texture_kind kind, voe_render_blocks blocks,
			  uint32_t width, uint32_t height, uint32_t level_count,
			  const uint8_t *bytes, voe_render_texture *out,
			  voe_base_error *error)
{
	VkBufferImageCopy regions[32] = { 0 };
	struct voe_render_texture_slot *slot;
	VkDeviceSize offset = 0;

	VOE_BASE_DEBUG_ASSERT(bytes != NULL, "creating a texture from nothing");
	VOE_BASE_DEBUG_ASSERT(out != NULL, "creating a texture into nothing");
	VOE_BASE_ASSERT(width > 0 && height > 0,
			"creating a texture with no pixels in it");
	VOE_BASE_ASSERT(level_count >= 1 &&
				level_count <=
					voe_render_texture_full_levels(width, height),
			"a texture given no levels, or more than its full chain");

	if (!device->bc) {
		VOE_BASE_ERROR("render",
			       "a %ux%u compressed texture refused: this graphics card has no textureCompressionBC",
			       width, height);
		if (error != NULL)
			*error = VOE_BASE_ERROR_REFUSED;
		return false;
	}

	slot = voe_render_texture_slot_find(device, error);
	if (slot == NULL)
		return false;

	for (uint32_t i = 0; i < level_count; i++) {
		uint32_t w = width >> i > 0 ? width >> i : 1;
		uint32_t h = height >> i > 0 ? height >> i : 1;

		regions[i] = (VkBufferImageCopy){
			.bufferOffset = offset,
			.imageSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = i,
				.layerCount = 1,
			},
			.imageExtent = { w, h, 1 },
		};
		offset += (VkDeviceSize)((w + 3) / 4) * ((h + 3) / 4) * 16;
	}

	if (!voe_render_texture_fill(device, slot, blocks_format(kind, blocks),
				     width, height, level_count, bytes, offset,
				     regions)) {
		if (error != NULL)
			*error = VOE_BASE_ERROR_REFUSED;
		return false;
	}

	slot->blocks = true;
	voe_render_texture_slot_claim(device, slot, VOE_RENDER_SAMPLING_SMOOTH,
				      width, height, out);
	return true;
}

// Under the device's guard (ADR-0370), as voe_render_texture_create is.
bool voe_render_texture_create_blocks(voe_render_device *device,
				      voe_render_texture_kind kind,
				      voe_render_blocks blocks, uint32_t width,
				      uint32_t height, uint32_t level_count,
				      const uint8_t *bytes,
				      voe_render_texture *out,
				      voe_base_error *error)
{
	bool created;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "creating a texture with no device");

	voe_render_device_guard_take(device);
	created = create_blocks(device, kind, blocks, width, height,
				level_count, bytes, out, error);
	voe_render_device_guard_give(device);
	VOE_BASE_DEBUG_ASSERT(!created || out->index != VOE_RENDER_NO_TEXTURE,
			      "a texture created into the default slot");
	return created;
}
