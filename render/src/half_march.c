// The half-size march and full-size composite the clouds and the fog share
// (ADR-0492 point 5, 0493 point 5): voe_render_frame_half_march, declared in
// frame_internal.h, called by clouds_draw.c and fog_draw.c with their own
// march and composite pipelines and span name, once their asserts and skip
// have passed.
//
// THE ORDER, all inside the span: the depth copy (depth_copy.c) when the pass
// has not made one, since the march reaches to the depth drawn; the pass's
// block ended; this frame slot's clouds image to a colour attachment; a block
// of its own onto it at half size, the march pipeline, three vertices; the
// image back to shader read; the pass's block reopened loading colour and
// depth through pass.c's voe_render_open_rendering, which sets the viewport
// and scissor as before; `clouds_image` written; the composite pipeline,
// three vertices.
//
// THE BLOCK SPLITS because the march draws into another image than the pass's,
// and an attachment is fixed for a rendering block's life. The barrier after
// the march also makes the first block's colour and depth writes visible to
// the block that loads them. The bound pipeline and set outlive the split.
//
// ONE IMAGE, TWO MARCHES: the clouds and then the fog march into the target's
// one clouds image, so the fog's march writes over what the clouds' composite
// read. The barrier into attachment waits on the fragment stage's sampled
// reads before the layout moves, which orders the second use after the first
// composite; the image's contents are discarded, as the march writes it whole.
//
// THE SLOT IS WRITTEN INTO THE PASS'S ONE BLOCK of the slot's uniform buffer,
// as the depth copy's is, because that block is where every draw of the pass
// reads the pass from: the composites read `clouds_image` there, and no other
// draw reads it. It stays VOE_RENDER_NO_CLOUDS_IMAGE until the first call.
//
// IT PUSHES A ZEROED PUSH CONSTANT before each draw, as sky_draw.c does, so the
// composite exposes and tones as the pass says whatever a mesh draw pushed.
//
// IT LEAVES `bound` AS THE COMPOSITE PIPELINE, which no mesh draw uses, so the
// next mesh draw binds its own and records its cull mode again (draw.c). Neither
// draw is counted in voe_render_frame_draw_count, which counts the caller's.
//
// Constraints: only inside an open camera pass, which the callers assert; the
// pipelines must be made. False only when the depth copy is.
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
			      "handing the clouds image back without the pass's writes");
}

// The march: a block onto `clouds` at `half`, `pipeline`, three vertices, the
// block ended. The image is written whole, so it neither loads nor clears.
static void march(voe_render_device *device, VkCommandBuffer commands,
		  VkPipeline pipeline,
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
			      "marching into no pixels");
	voe_render_vk.cmd_begin_rendering(commands, &rendering);
	voe_render_vk.cmd_set_viewport(commands, 0, 1, &viewport);
	voe_render_vk.cmd_set_scissor(commands, 0, 1, &scissor);
	voe_render_vk.cmd_bind_pipeline(commands, VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipeline);
	device->bound = pipeline;
	voe_render_vk.cmd_push_constants(commands, device->layout,
					 VK_SHADER_STAGE_VERTEX_BIT |
						 VK_SHADER_STAGE_FRAGMENT_BIT,
					 0, sizeof(push), push);
	voe_render_vk.cmd_draw(commands, 3, 1, 0, 0);
	voe_render_vk.cmd_end_rendering(commands);
}

bool voe_render_frame_half_march(voe_render_device *device,
				 VkPipeline march_pipeline,
				 VkPipeline composite_pipeline,
				 const char *span)
{
	struct voe_render_frame *frame;
	struct voe_render_frame_block *block;
	const struct voe_render_target *images;
	const uint32_t push[5] = { 0 };
	uint32_t slot;
	VkExtent2D extent;

	VOE_BASE_DEBUG_ASSERT(march_pipeline != VK_NULL_HANDLE &&
				      composite_pipeline != VK_NULL_HANDLE,
			      "marching on a device not prepared");
	VOE_BASE_DEBUG_ASSERT(span != NULL, "marching in no span");
	frame = voe_render_frame_at(device, device->slot);
	// The open pass's block is the one before pass_count.
	block = (struct voe_render_frame_block *)((unsigned char *)
							  frame->uniforms_mapped +
						  (device->pass_count - 1) *
							  device->pass_stride);
	if (device->pass_target == NULL) {
		images = &frame->target;
		slot = device->window_clouds_texture;
	} else {
		images = &device->pass_target->images[device->slot];
		slot = device->pass_target->clouds_texture;
	}
	extent = device->pass_extent;
	VOE_BASE_DEBUG_ASSERT(slot != 0, "marching with no clouds slot");

	voe_render_frame_span_begin(device, span);
	if (block->depth_copy == VOE_RENDER_NO_DEPTH_COPY &&
	    !voe_render_frame_copy_depth(device)) {
		voe_render_frame_span_end(device);
		return false;
	}
	voe_render_vk.cmd_end_rendering(frame->commands);
	clouds_barriers(frame->commands, images->clouds.image, true);
	march(device, frame->commands, march_pipeline, &images->clouds,
	      (VkExtent2D){ (extent.width + 1) / 2, (extent.height + 1) / 2 });
	clouds_barriers(frame->commands, images->clouds.image, false);
	voe_render_open_rendering(frame->commands, images, extent, false,
				  device->pass_target != NULL);

	block->clouds_image = slot;
	voe_render_vk.cmd_bind_pipeline(frame->commands,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					composite_pipeline);
	device->bound = composite_pipeline;
	voe_render_vk.cmd_push_constants(frame->commands, device->layout,
					 VK_SHADER_STAGE_VERTEX_BIT |
						 VK_SHADER_STAGE_FRAGMENT_BIT,
					 0, sizeof(push), push);
	voe_render_vk.cmd_draw(frame->commands, 3, 1, 0, 0);
	voe_render_frame_span_end(device);
	return true;
}
