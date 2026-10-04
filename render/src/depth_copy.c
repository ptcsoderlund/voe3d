// The depth copy (ADR-0305): voe_render_frame_copy_depth copies an open camera
// pass's depth into the sampled copy beside it, the window's or a target's of
// the caller's own, so a later draw of the pass can read the depth drawn so far.
//
// ONE BLOCK BECOMES TWO, AND THE SECOND LOADS. The copy cannot happen inside a
// rendering block, so the pass's block ends, the copy runs between barriers, and
// a block that loads colour and depth reopens through pass.c's
// voe_render_open_rendering; the bound pipeline, set and pools outlive the split.
//
// THE SLOT IS WRITTEN INTO THE PASS'S ONE BLOCK of the slot's uniform buffer, so
// every draw of the pass reads it, those before the copy too.
//
// Called by the user inside an open camera pass; a shadow, point-shadow,
// capture or bounce shadow pass, or no pass, is refused with an error and false.
#include "frame_internal.h"

#include <base/assert.h>
#include <base/report.h>

// The depth image to TRANSFER_SRC and its copy to TRANSFER_DST after the block's
// last depth write and any earlier read of the copy (`before`), or both back,
// the copy to where shaders read it and the colour attachment's writes made
// visible to the block that loads it (!`before`).
static void depth_copy_barriers(VkCommandBuffer commands,
				const struct voe_render_target *images,
				bool before)
{
	const VkImageSubresourceRange depth_range = {
		.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
		.levelCount = 1,
		.layerCount = 1,
	};
	const VkPipelineStageFlags2 tests =
		VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
		VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
	const VkPipelineStageFlags2 shaders =
		VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
		VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	const VkAccessFlags2 attachment =
		VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
		VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	VkImageMemoryBarrier2 barriers[2] = {
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = tests,
			.srcAccessMask = attachment,
			.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
			.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = images->depth.image,
			.subresourceRange = depth_range,
		},
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = shaders,
			.srcAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
			.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = images->depth_copy.image,
			.subresourceRange = depth_range,
		},
	};
	VkMemoryBarrier2 colour = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT |
				 VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
	};
	VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 2,
		.pImageMemoryBarriers = barriers,
	};

	VOE_BASE_DEBUG_ASSERT(images != NULL, "copying depth of no target");
	VOE_BASE_DEBUG_ASSERT(images->depth_copy.image != VK_NULL_HANDLE,
			      "copying depth into a target with no depth copy");

	if (!before) {
		barriers[0] = (VkImageMemoryBarrier2){
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
			.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
			.dstStageMask = tests,
			.dstAccessMask = attachment,
			.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = images->depth.image,
			.subresourceRange = depth_range,
		};
		barriers[1] = (VkImageMemoryBarrier2){
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
			.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
			.dstStageMask = shaders,
			.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = images->depth_copy.image,
			.subresourceRange = depth_range,
		};
		dependency.memoryBarrierCount = 1;
		dependency.pMemoryBarriers = &colour;
	}
	voe_render_vk.cmd_pipeline_barrier2(commands, &dependency);
}

// ONE BLOCK BECOMES TWO, AND THE SECOND LOADS. The draws after the copy are in
// a rendering block of their own that loads colour and depth, which is what
// keeps what came before; the bound pipeline, set and pools outlive the split,
// and the viewport and scissor are set again by the reopen.
//
// THE SLOT IS WRITTEN INTO THE PASS'S ONE BLOCK, so every draw of the pass reads
// it, those before the copy too. Nothing reads it yet; the reader that comes
// will draw only after the copy.
bool voe_render_frame_copy_depth(voe_render_device *device)
{
	struct voe_render_frame *frame;
	const struct voe_render_target *images;
	struct voe_render_frame_block *block;
	VkExtent2D extent;
	uint32_t texture;
	VkImageCopy region = {
		.srcSubresource = { .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
				    .layerCount = 1 },
		.dstSubresource = { .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
				    .layerCount = 1 },
	};

	VOE_BASE_ASSERT(device != NULL, "copying depth on no device");
	if (!device->pass_open || !device->pass_camera || device->pass_shadow ||
	    device->pass_point_shadow || device->pass_capture ||
	    device->pass_bounce_shadow) {
		VOE_BASE_ERROR("render",
			       "copying depth outside an open camera pass — a shadow or capture pass's depth is the map itself");
		return false;
	}

	frame = voe_render_frame_at(device, device->slot);
	if (device->pass_target == NULL) {
		images = &frame->target;
		texture = device->window_depth_texture;
	} else {
		images = &device->pass_target->images[device->slot];
		texture = device->pass_target->depth_texture;
	}
	extent = device->pass_extent;
	region.extent = (VkExtent3D){ extent.width, extent.height, 1 };
	VOE_BASE_DEBUG_ASSERT(texture != 0, "copying depth with no copy slot");

	voe_render_vk.cmd_end_rendering(frame->commands);
	depth_copy_barriers(frame->commands, images, true);
	voe_render_vk.cmd_copy_image(frame->commands, images->depth.image,
				     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				     images->depth_copy.image,
				     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
				     &region);
	depth_copy_barriers(frame->commands, images, false);
	voe_render_open_rendering(frame->commands, images, extent, false,
				  device->pass_target != NULL);

	// The open pass's block is the one before pass_count, voe_render_pass_start's.
	block = (struct voe_render_frame_block *)((unsigned char *)
							  frame->uniforms_mapped +
						  (device->pass_count - 1) *
							  device->pass_stride);
	block->depth_copy = texture;
	return true;
}
