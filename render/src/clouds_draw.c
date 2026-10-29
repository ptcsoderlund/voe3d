// The clouds drawn over the world (ADR-0492 point 5):
// voe_render_frame_draw_clouds, declared in render/device.h, marches the open
// camera pass's clouds into its target's half-size clouds image and composites
// them over what the pass holds. 3d's run calls it after the world's blended
// draws and before the overlay's depth clear, once a pass. The shaders are
// draw_clouds.slangh's march and composite behind the sky's vertex stage.
//
// THE ORDER, all inside the span `clouds`: the depth copy (depth_copy.c) when
// the pass has not made one, since the march reaches to the depth drawn; the
// pass's block ended; this frame slot's clouds image to a colour attachment;
// a block of its own onto it at half size, the march pipeline, three
// vertices; the image back to shader read; the pass's block reopened loading
// colour and depth through pass.c's voe_render_open_rendering, which sets the
// viewport and scissor as before; the composite pipeline, three vertices.
//
// THE BLOCK SPLITS because the march draws into another image than the pass's,
// and an attachment is fixed for a rendering block's life. The barrier after
// the march also makes the first block's colour and depth writes visible to
// the block that loads them. The bound pipeline and set outlive the split.
//
// THE SLOT IS WRITTEN INTO THE PASS'S ONE BLOCK of the slot's uniform buffer,
// as the depth copy's is, because that block is where every draw of the pass
// reads the pass from: the composite reads `clouds_image` there, and no other
// draw reads it. It stays VOE_RENDER_NO_CLOUDS_IMAGE until this call.
//
// IT PUSHES A ZEROED PUSH CONSTANT before each draw, as sky_draw.c does, so the
// composite exposes and tones as the pass says whatever a mesh draw pushed.
//
// IT LEAVES `bound` AS THE COMPOSITE PIPELINE, which no mesh draw uses, so the
// next mesh draw binds its own and records its cull mode again (draw.c). Neither
// draw is counted in voe_render_frame_draw_count, which counts the caller's.
//
// Constraints: a pass whose clouds kind is none records nothing; no pass, no
// camera, or a shadow, point-shadow or capture pass asserts. The kind is read
// back from the pass's block in mapped, possibly uncached memory: four bytes.
#include "frame_internal.h"

#include <base/assert.h>

#include <stdint.h>

// This frame slot's clouds image of the open pass's target from where shaders
// read it to a colour attachment the march writes whole (`before`), or back,
// with the first block's colour and depth writes made visible to the block
// that loads them (!`before`).
static void clouds_barriers(VkCommandBuffer commands, VkImage clouds,
			    bool before)
{
	const VkPipelineStageFlags2 attachments =
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT |
		VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
		VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
	VkImageMemoryBarrier2 image = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
		.srcAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = clouds,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};
	VkMemoryBarrier2 written = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
		.srcStageMask = attachments,
		.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT |
				 VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		.dstStageMask = attachments,
		.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT |
				 VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT |
				 VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
				 VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
	};
	VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &image,
	};

	VOE_BASE_DEBUG_ASSERT(clouds != VK_NULL_HANDLE,
			      "moving no clouds image");
	if (!before) {
		image.srcStageMask =
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		image.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		image.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		image.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
		image.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		image.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		dependency.memoryBarrierCount = 1;
		dependency.pMemoryBarriers = &written;
	}
	voe_render_vk.cmd_pipeline_barrier2(commands, &dependency);
	VOE_BASE_DEBUG_ASSERT(before || dependency.memoryBarrierCount == 1,
			      "handing the clouds back without the pass's writes");
}

// The march: a block onto `clouds` at `half`, the march pipeline, three
// vertices, the block ended. The image is written whole, so it neither loads
// nor clears.
static void march(voe_render_device *device, VkCommandBuffer commands,
		  const struct voe_render_allocated_image *clouds,
		  VkExtent2D half)
{
	const uint32_t push[5] = { 0 };
	VkRenderingAttachmentInfo colour = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = clouds->view,
		.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
	};
	VkRenderingInfo rendering = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea = { .extent = half },
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colour,
	};
	const VkRect2D scissor = { .extent = half };
	const VkViewport viewport = voe_render_frame_viewport(half);

	VOE_BASE_DEBUG_ASSERT(half.width > 0 && half.height > 0,
			      "marching clouds into no pixels");
	voe_render_vk.cmd_begin_rendering(commands, &rendering);
	voe_render_vk.cmd_set_viewport(commands, 0, 1, &viewport);
	voe_render_vk.cmd_set_scissor(commands, 0, 1, &scissor);
	voe_render_vk.cmd_bind_pipeline(commands, VK_PIPELINE_BIND_POINT_GRAPHICS,
					device->pipeline_clouds_march);
	device->bound = device->pipeline_clouds_march;
	voe_render_vk.cmd_push_constants(commands, device->layout,
					 VK_SHADER_STAGE_VERTEX_BIT |
						 VK_SHADER_STAGE_FRAGMENT_BIT,
					 0, sizeof(push), push);
	voe_render_vk.cmd_draw(commands, 3, 1, 0, 0);
	voe_render_vk.cmd_end_rendering(commands);
}

bool voe_render_frame_draw_clouds(voe_render_device *device)
{
	struct voe_render_frame *frame;
	struct voe_render_frame_block *block;
	const struct voe_render_target *images;
	const uint32_t push[5] = { 0 };
	uint32_t slot;
	VkExtent2D extent;

	VOE_BASE_ASSERT(device != NULL, "drawing clouds on no device");
	VOE_BASE_ASSERT(device->pass_open && device->pass_count > 0,
			"drawing clouds with no pass open");
	VOE_BASE_ASSERT(device->pass_camera,
			"drawing clouds in a pass opened with no camera");
	VOE_BASE_ASSERT(!device->pass_shadow && !device->pass_bounce_shadow &&
				!device->pass_point_shadow && !device->pass_capture,
			"drawing clouds in a shadow, point-shadow or capture pass");

	frame = voe_render_frame_at(device, device->slot);
	// The open pass's block is the one before pass_count.
	block = (struct voe_render_frame_block *)((unsigned char *)
							  frame->uniforms_mapped +
						  (device->pass_count - 1) *
							  device->pass_stride);
	if (block->clouds.kind == VOE_RENDER_CLOUDS_NONE)
		return true;
	VOE_BASE_DEBUG_ASSERT(device->pipeline_clouds_march != VK_NULL_HANDLE &&
				      device->pipeline_clouds_composite !=
					      VK_NULL_HANDLE,
			      "drawing clouds on a device not prepared");
	if (device->pass_target == NULL) {
		images = &frame->target;
		slot = device->window_clouds_texture;
	} else {
		images = &device->pass_target->images[device->slot];
		slot = device->pass_target->clouds_texture;
	}
	extent = device->pass_extent;
	VOE_BASE_DEBUG_ASSERT(slot != 0, "drawing clouds with no clouds slot");

	voe_render_frame_span_begin(device, "clouds");
	if (block->depth_copy == VOE_RENDER_NO_DEPTH_COPY &&
	    !voe_render_frame_copy_depth(device)) {
		voe_render_frame_span_end(device);
		return false;
	}
	voe_render_vk.cmd_end_rendering(frame->commands);
	clouds_barriers(frame->commands, images->clouds.image, true);
	march(device, frame->commands, &images->clouds,
	      (VkExtent2D){ (extent.width + 1) / 2, (extent.height + 1) / 2 });
	clouds_barriers(frame->commands, images->clouds.image, false);
	voe_render_open_rendering(frame->commands, images, extent, false,
				  device->pass_target != NULL);

	block->clouds_image = slot;
	voe_render_vk.cmd_bind_pipeline(frame->commands,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					device->pipeline_clouds_composite);
	device->bound = device->pipeline_clouds_composite;
	voe_render_vk.cmd_push_constants(frame->commands, device->layout,
					 VK_SHADER_STAGE_VERTEX_BIT |
						 VK_SHADER_STAGE_FRAGMENT_BIT,
					 0, sizeof(push), push);
	voe_render_vk.cmd_draw(frame->commands, 3, 1, 0, 0);
	voe_render_frame_span_end(device);
	return true;
}
