// The sun's bounce map (ADR-0308 point 1): per frame slot a D32 depth image and
// two RGBA16F colour images, flux and normal, VOE_RENDER_BOUNCE_TEXELS square;
// the rendering a bounce pass draws in, cleared; and the barriers that hand all
// three to a compute shader once it ends. pass.c opens and closes bounce passes;
// this file owns the images and their layouts.
//
// LIFETIME: STARTUP TO SHUTDOWN, like shadow.c's maps: the size is a constant, so
// no resize touches it. open_device builds it after the frame objects, whose
// command pool settles it.
//
// PER SLOT, because a frame in flight may still be reading its slot's map while
// the next frame draws into its own. The slot's fence makes each slot's safe.
//
// THE RESTING LAYOUTS ARE WHAT COMPUTE READS: the colour images in GENERAL, the
// one layout valid for a sampled and a storage read alike; depth in
// SHADER_READ_ONLY_OPTIMAL, sampled. Startup settles them there; a bounce pass
// moves all three to their attachment layouts out of UNDEFINED, because it
// clears what was there, and back at its end.
#include "device_internal.h"

#include <base/assert.h>
#include <base/report.h>

#define BOUNCE_COLOUR_FORMAT VK_FORMAT_R16G16B16A16_SFLOAT

// One barrier on `image`, into its attachment layout (`in`) or back to where
// compute reads it. The stages either side follow from the aspect.
static VkImageMemoryBarrier2 bounce_barrier(VkImage image, bool depth, bool in)
{
	const VkPipelineStageFlags2 tests =
		VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
		VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
	const VkPipelineStageFlags2 draw =
		depth ? tests : VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	const VkAccessFlags2 write = depth ?
		VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
			VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT :
		VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	const VkImageLayout attachment =
		depth ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL :
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	const VkImageLayout rest = depth ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL :
					   VK_IMAGE_LAYOUT_GENERAL;
	const VkAccessFlags2 read = depth ?
		VK_ACCESS_2_SHADER_SAMPLED_READ_BIT :
		VK_ACCESS_2_SHADER_SAMPLED_READ_BIT |
			VK_ACCESS_2_SHADER_STORAGE_READ_BIT;

	return (VkImageMemoryBarrier2){
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = in ? VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT : draw,
		.srcAccessMask = in ? VK_ACCESS_2_NONE : write,
		.dstStageMask = in ? draw : VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		.dstAccessMask = in ? write : read,
		.oldLayout = in ? VK_IMAGE_LAYOUT_UNDEFINED : attachment,
		.newLayout = in ? attachment : rest,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {
			.aspectMask = depth ? VK_IMAGE_ASPECT_DEPTH_BIT :
					      VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};
}

// The three barriers of one slot's map, in or back, recorded as one.
static void record_barriers(const struct voe_render_frame *frame, bool in)
{
	VkImageMemoryBarrier2 barriers[3] = {
		bounce_barrier(frame->bounce.depth.image, true, in),
		bounce_barrier(frame->bounce.flux.image, false, in),
		bounce_barrier(frame->bounce.normal.image, false, in),
	};
	VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 3,
		.pImageMemoryBarriers = barriers,
	};

	voe_render_vk.cmd_pipeline_barrier2(frame->commands, &dependency);
}

bool voe_render_bounce_startup(voe_render_device *device)
{
	const VkExtent2D extent = { VOE_RENDER_BOUNCE_TEXELS,
				    VOE_RENDER_BOUNCE_TEXELS };
	const VkImageUsageFlags colour = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
					 VK_IMAGE_USAGE_SAMPLED_BIT |
					 VK_IMAGE_USAGE_STORAGE_BIT;
	VkImageMemoryBarrier2 barriers[3 * VOE_RENDER_FRAMES_IN_FLIGHT];

	VOE_BASE_ASSERT(device != NULL, "making bounce maps on no device");
	VOE_BASE_ASSERT(device->pool != VK_NULL_HANDLE,
			"making bounce maps before the command pool that settles them");

	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		struct voe_render_bounce_map *map = &device->frames[i].bounce;

		if (!voe_render_target_image_build(device, &map->depth, extent,
						   VOE_RENDER_DEPTH_FORMAT,
						   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
							   VK_IMAGE_USAGE_SAMPLED_BIT,
						   VK_IMAGE_ASPECT_DEPTH_BIT,
						   "bounce depth") ||
		    !voe_render_target_image_build(device, &map->flux, extent,
						   BOUNCE_COLOUR_FORMAT, colour,
						   VK_IMAGE_ASPECT_COLOR_BIT,
						   "bounce flux") ||
		    !voe_render_target_image_build(device, &map->normal, extent,
						   BOUNCE_COLOUR_FORMAT, colour,
						   VK_IMAGE_ASPECT_COLOR_BIT,
						   "bounce normal"))
			return false;

		// Out of UNDEFINED straight to where each rests: the "back" barrier
		// with its old layout replaced.
		barriers[3 * i] = bounce_barrier(map->depth.image, true, false);
		barriers[3 * i + 1] = bounce_barrier(map->flux.image, false, false);
		barriers[3 * i + 2] = bounce_barrier(map->normal.image, false, false);
	}
	for (uint32_t i = 0; i < 3 * VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		barriers[i].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barriers[i].srcAccessMask = VK_ACCESS_2_NONE;
	}
	return voe_render_target_settle(device, barriers,
					3 * VOE_RENDER_FRAMES_IN_FLIGHT);
}

void voe_render_bounce_shutdown(voe_render_device *device)
{
	VOE_BASE_ASSERT(device != NULL, "taking bounce maps from no device");

	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		struct voe_render_bounce_map *map = &device->frames[i].bounce;

		voe_render_target_image_teardown(device, &map->depth);
		voe_render_target_image_teardown(device, &map->flux);
		voe_render_target_image_teardown(device, &map->normal);
	}
}

// Flux and normal cleared to nought, so a texel no caster covers carries no
// light; depth to the far plane, as every pass's is.
void voe_render_bounce_open(const struct voe_render_frame *frame)
{
	const VkExtent2D extent = { VOE_RENDER_BOUNCE_TEXELS,
				    VOE_RENDER_BOUNCE_TEXELS };
	VkRenderingAttachmentInfo colours[2] = {
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = frame->bounce.flux.view,
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		},
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = frame->bounce.normal.view,
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		},
	};
	VkRenderingAttachmentInfo depth = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = frame->bounce.depth.view,
		.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = { .depthStencil = { .depth = VOE_RENDER_DEPTH_CLEAR } },
	};
	VkRenderingInfo rendering = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea = { .extent = extent },
		.layerCount = 1,
		.colorAttachmentCount = 2,
		.pColorAttachments = colours,
		.pDepthAttachment = &depth,
	};
	VkRect2D scissor = { .extent = extent };
	VkViewport viewport = voe_render_frame_viewport(extent);

	VOE_BASE_ASSERT(frame != NULL, "opening a bounce map on no frame slot");

	record_barriers(frame, true);
	voe_render_vk.cmd_begin_rendering(frame->commands, &rendering);
	voe_render_vk.cmd_set_viewport(frame->commands, 0, 1, &viewport);
	voe_render_vk.cmd_set_scissor(frame->commands, 0, 1, &scissor);
}

void voe_render_bounce_to_read(const struct voe_render_frame *frame)
{
	VOE_BASE_ASSERT(frame != NULL, "a bounce barrier on no frame slot");
	record_barriers(frame, false);
}
