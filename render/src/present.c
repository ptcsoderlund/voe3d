// Present: the one write into a swapchain image, and the last thing a frame
// records. voe_render_frame_end calls voe_render_ready_for_copy once the last
// rendering block has ended, then voe_render_blit_to_screen when there is a
// window; a headless device stops after the first, and a test copies the
// picture out of the target itself.
//
// NOTHING ELSE DRAWS INTO A SWAPCHAIN IMAGE. The scene goes into images the
// engine owns (target.c) and the swapchain image is written once, by the blit
// here. That separation is what a post-process pass, a render resolution
// different from the window's, and an editor viewport all need in order to
// exist at all.
#include "frame_internal.h"

// Leaves the colour image ready to be copied out of, by whoever asked for the
// drawing, once the last rendering block has ended. A frame blits it into a
// swapchain image; a test copies it into memory it can read. ALL_TRANSFER and
// not the blit alone, because those are two different stages and this barrier
// has to cover both — naming one of them leaves the other reading an image this
// dependency does not reach, which synchronisation validation reports and
// nothing else does.
//
// The depth image gets no second barrier: nothing reads it after the frame, so
// there is no later access for one to order against.
void voe_render_ready_for_copy(const struct voe_render_frame *frame)
{
	VkImageMemoryBarrier2 barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,
		.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = frame->target.colour.image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};
	VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &barrier,
	};

	voe_render_vk.cmd_pipeline_barrier2(frame->commands, &dependency);
}

// The target onto the screen, and the whole of what the swapchain image is for.
// The two extents are the same number today and the blit still reads both, so
// that the day the target stops being the window's size this needs no edit.
//
// THE COPY IS A BLIT AND THAT IS A STARTING POINT. vkCmdBlitImage is one call
// and it scales, which is everything this needs. A full-screen quad becomes
// necessary the moment anything wants to run a shader between the target and the
// screen — tone mapping first — and that is the card that replaces this.
void voe_render_blit_to_screen(voe_render_device *device,
			       const struct voe_render_frame *frame,
			       const struct voe_render_image *image)
{
	VkImageMemoryBarrier2 barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
		.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image->image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};
	VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &barrier,
	};
	VkImageBlit region = {
		.srcSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.layerCount = 1,
		},
		.srcOffsets = { { 0, 0, 0 },
				{ (int32_t)device->resolution.width,
				  (int32_t)device->resolution.height, 1 } },
		.dstSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.layerCount = 1,
		},
		.dstOffsets = { { 0, 0, 0 },
				{ (int32_t)device->extent.width,
				  (int32_t)device->extent.height, 1 } },
	};

	voe_render_vk.cmd_pipeline_barrier2(frame->commands, &dependency);

	// NEAREST, which does nothing at all while the two extents match and is
	// the engine's rule the moment they do not. A linear blit is a smoothing
	// filter over the whole finished frame — the largest piece of
	// antialiasing there could be here — and this engine does not want
	// antialiasing anywhere. A render resolution below the window's will
	// therefore show square pixels rather than a blur, which is the intended
	// picture and not a fault in the blit.
	voe_render_vk.cmd_blit_image(frame->commands, frame->target.colour.image,
				     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				     image->image,
				     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
				     &region, VK_FILTER_NEAREST);

	barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
	barrier.dstAccessMask = 0;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	voe_render_vk.cmd_pipeline_barrier2(frame->commands, &dependency);
}
