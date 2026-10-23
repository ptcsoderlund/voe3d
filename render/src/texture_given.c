// A SMOOTH texture whose mip chain comes from the caller:
// voe_render_texture_create_levels, declared in render/include/render/device.h.
//
// THE LEVELS COME FROM THE CALLER BECAUSE A COOKED PICTURE ALREADY HAS THEM
// (ADR-0470 point 3). Its chain was made once on the CPU, by the area average of
// its kind, and stored in the cooked copy; building another on the GPU would
// throw that work away and give a normal or data map a blit's average instead.
// So the image is made with exactly the levels given and none is built: one
// level given is an image of one level.
//
// ONE STAGING BUFFER HOLDS EVERY LEVEL, AND ONE COPY FILLS THEM ALL. The levels
// sit in `rgba` one after another, as the cooked copy stores them, so the
// staging buffer is that block as it is. One region per level names where its
// level starts in the buffer and its size, and the one copy command takes every
// region; texture.c's upload records it, moves every level to the shader and
// idles, as it does for _create.
//
// Constraints: at most 32 levels, which is the full chain of a 32-bit side; the
// regions are on the stack for that reason.
#include "device_internal.h"

#include <base/assert.h>

static bool create_levels(voe_render_device *device,
			  voe_render_texture_kind kind, uint32_t width,
			  uint32_t height, uint32_t level_count,
			  const uint8_t *rgba, voe_render_texture *out,
			  voe_base_error *error)
{
	VkBufferImageCopy regions[32] = { 0 };
	struct voe_render_texture_slot *slot;
	VkDeviceSize offset = 0;

	VOE_BASE_DEBUG_ASSERT(rgba != NULL, "creating a texture from nothing");
	VOE_BASE_DEBUG_ASSERT(out != NULL, "creating a texture into nothing");
	VOE_BASE_ASSERT(width > 0 && height > 0,
			"creating a texture with no pixels in it");
	VOE_BASE_ASSERT(level_count >= 1 &&
				level_count <=
					voe_render_texture_full_levels(width, height),
			"a texture given no levels, or more than its full chain");

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
		offset += (VkDeviceSize)w * h * 4;
	}

	if (!voe_render_texture_fill(device, slot,
				     voe_render_texture_format(kind), width,
				     height, level_count, rgba, offset,
				     regions)) {
		if (error != NULL)
			*error = VOE_BASE_ERROR_REFUSED;
		return false;
	}

	voe_render_texture_slot_claim(device, slot, VOE_RENDER_SAMPLING_SMOOTH,
				      width, height, out);
	return true;
}

// Under the device's guard (ADR-0370), as voe_render_texture_create is.
bool voe_render_texture_create_levels(voe_render_device *device,
				      voe_render_texture_kind kind,
				      uint32_t width, uint32_t height,
				      uint32_t level_count, const uint8_t *rgba,
				      voe_render_texture *out,
				      voe_base_error *error)
{
	bool created;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "creating a texture with no device");

	voe_render_device_guard_take(device);
	created = create_levels(device, kind, width, height, level_count, rgba,
				out, error);
	voe_render_device_guard_give(device);
	VOE_BASE_DEBUG_ASSERT(!created || out->index != VOE_RENDER_NO_TEXTURE,
			      "a texture created into the default slot");
	return created;
}
