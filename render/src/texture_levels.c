// A texture's mip chain, built on the GPU from the level the upload filled
// (ADR-0359). texture.c calls voe_render_texture_levels_record for a SMOOTH
// texture, after the staging copy and before the command buffer ends.
//
// A BLIT AND NOT A CPU DOWNSCALE. The colour textures are R8G8B8A8_SRGB, and a
// linear blit out of an sRGB image decodes, averages and encodes again, so each
// level is the linear-light average of the four texels above it: a black and
// white checker becomes 0.5 in light, not 0.5 in bytes. A CPU loop would have to
// do that curve by hand. R8G8B8A8 in SRGB and UNORM is required by the spec to
// support linear-filtered blits as source and destination, so there is no
// format to query and no fallback.
//
// IT RUNS INSIDE THE STARTUP UPLOAD'S ONE SUBMISSION. Nothing here allocates,
// submits or waits: it records into the caller's command buffer, which
// texture.c submits and idles on as it always did.
//
// Constraints: level 0 in TRANSFER_DST_OPTIMAL holding the picture and every
// other level in TRANSFER_DST_OPTIMAL too (the caller's first transition covers
// them all); the image made with TRANSFER_SRC and TRANSFER_DST usage and
// `level_count` levels. Every level leaves in SHADER_READ_ONLY_OPTIMAL.
#include "device_internal.h"

#include <base/assert.h>

// One level from one layout to another, ordered after the transfer before it
// and before whatever reads it next: the next blit, or the fragment stage.
static void move_level(VkCommandBuffer commands, VkImage image, uint32_t level,
		       VkImageLayout from, VkImageLayout to)
{
	VkImageMemoryBarrier2 barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT |
				VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
		.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT |
				 VK_ACCESS_2_SHADER_READ_BIT,
		.oldLayout = from,
		.newLayout = to,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = level,
			.levelCount = 1,
			.layerCount = 1,
		},
	};
	VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &barrier,
	};

	voe_render_vk.cmd_pipeline_barrier2(commands, &dependency);
}

void voe_render_texture_levels_record(voe_render_device *device,
				      VkCommandBuffer commands, VkImage image,
				      uint32_t width, uint32_t height,
				      uint32_t level_count)
{
	int32_t from_w = (int32_t)width;
	int32_t from_h = (int32_t)height;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "building levels on no device");
	VOE_BASE_DEBUG_ASSERT(commands != VK_NULL_HANDLE && image != VK_NULL_HANDLE,
			      "building levels with no command buffer or image");
	VOE_BASE_DEBUG_ASSERT(level_count >= 1 && level_count <= 32,
			      "a level count no 2D image of 32-bit sides can have");

	// Each pass reads level - 1 and writes level; the source is done with
	// after its blit and goes to the shader at once.
	for (uint32_t level = 1; level < level_count; level++) {
		int32_t to_w = from_w > 1 ? from_w / 2 : 1;
		int32_t to_h = from_h > 1 ? from_h / 2 : 1;
		VkImageBlit blit = {
			.srcSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = level - 1,
				.layerCount = 1,
			},
			.srcOffsets = { { 0, 0, 0 }, { from_w, from_h, 1 } },
			.dstSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = level,
				.layerCount = 1,
			},
			.dstOffsets = { { 0, 0, 0 }, { to_w, to_h, 1 } },
		};

		move_level(commands, image, level - 1,
			   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		voe_render_vk.cmd_blit_image(commands, image,
					     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					     image,
					     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					     1, &blit, VK_FILTER_LINEAR);
		move_level(commands, image, level - 1,
			   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		from_w = to_w;
		from_h = to_h;
	}

	// The last level was only ever written.
	move_level(commands, image, level_count - 1,
		   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VOE_BASE_DEBUG_ASSERT(from_w >= 1 && from_h >= 1,
			      "a level smaller than one texel");
}
