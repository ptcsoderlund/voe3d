// The bounce shadow pass (ADR-0329 points 2 and 3): the sun's depth map the
// relight shadows by, drawn as a shadow pass is but onto a map of its own.
// voe_render_bounce_shadow_pass_begin opens it after the begun target's capture
// passes; voe_render_pass_end calls voe_render_bounce_shadow_end once its
// rendering has ended. Draws in it take the shadow pass's path (draw.c).
//
// WHY A MAP OF ITS OWN. The cascades follow the view, so a relight made while
// looking away found the room behind the camera outside them and lit it
// unshadowed. 3d fits this map to the probe volume, which stands on the eye, so
// it answers the same whichever way the camera faces.
//
// THE MAP. Per frame slot a D32 image VOE_RENDER_BOUNCE_SHADOW_TEXELS square, a
// depth attachment and sampled (and a transfer source, so a test may read it),
// with one view. Built at startup beside the capture scratch on a device with
// shaderOutputLayer, none without, and settled into SHADER_READ_ONLY_OPTIMAL, where
// it rests, so a descriptor naming it is valid on a frame that drew none. The slot
// keeps the light's view × projection and whether the current begin drew it:
// that flag is per begin (ADR-0330 point 1), cleared by each
// voe_render_bounce_begin, so every view the editor shows draws the map for its
// own volume, and a second open after one begin asserts.
//
// THE BEGIN opens only when the begun target will relight (card 04's call) and
// the begun sun bounces, shines and is shaded: the map from UNDEFINED into its
// attachment layout, depth cleared to the far plane, the shadow pipeline, the
// viewport and scissor at the map's side, `light` as the pass's camera. Its
// barrier out of UNDEFINED waits on the fragment tests and on the compute stage
// as well: the slot has one map, so a second view's pass follows the previous
// view's relight reading it this frame, and overwrites it only after. THE END
// moves it back to SHADER_READ_ONLY_OPTIMAL before a compute read.
//
// COST. 4 MiB of depth a frame slot, 8 MiB over two. At most one pass per begun
// target (ADR-0330 point 3) and one object per caster drawn into each; nothing
// on a settled frame.
#include "frame_internal.h"

#include <base/assert.h>
#include <base/report.h>

#define TEXELS VOE_RENDER_BOUNCE_SHADOW_TEXELS

bool voe_render_bounce_shadow_startup(voe_render_device *device)
{
	const VkExtent2D extent = { TEXELS, TEXELS };
	VkImageMemoryBarrier2 settles[VOE_RENDER_FRAMES_IN_FLIGHT];

	VOE_BASE_ASSERT(device != NULL, "making bounce sun maps on no device");
	if (!device->output_layer)
		return true;
	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		struct voe_render_allocated_image *map =
			&device->frames[i].bounce_shadow.map;

		if (!voe_render_target_image_build(
			    device, map, extent, VOE_RENDER_DEPTH_FORMAT,
			    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
				    VK_IMAGE_USAGE_SAMPLED_BIT |
				    VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			    VK_IMAGE_ASPECT_DEPTH_BIT, "bounce sun map"))
			return false;
		settles[i] = voe_render_target_settle_copy(map->image);
	}
	return voe_render_target_settle(device, settles,
					VOE_RENDER_FRAMES_IN_FLIGHT);
}

void voe_render_bounce_shadow_shutdown(voe_render_device *device)
{
	VOE_BASE_ASSERT(device != NULL, "taking bounce sun maps from no device");
	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++)
		voe_render_target_image_teardown(device,
						 &device->frames[i].bounce_shadow.map);
}

// The map between `from` and `to`, after `src` and before `dst` stages with
// their accesses.
static void move(const struct voe_render_frame *frame, VkImageLayout from,
		 VkImageLayout to, VkPipelineStageFlags2 src,
		 VkAccessFlags2 src_access, VkPipelineStageFlags2 dst,
		 VkAccessFlags2 dst_access)
{
	const VkImageMemoryBarrier2 barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = src,
		.srcAccessMask = src_access,
		.dstStageMask = dst,
		.dstAccessMask = dst_access,
		.oldLayout = from,
		.newLayout = to,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = frame->bounce_shadow.map.image,
		.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
				      .levelCount = 1,
				      .layerCount = 1 },
	};
	const VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &barrier,
	};

	VOE_BASE_DEBUG_ASSERT(barrier.image != VK_NULL_HANDLE,
			      "moving a bounce sun map that was never built");
	voe_render_vk.cmd_pipeline_barrier2(frame->commands, &dependency);
}

// Whether this frame's begun bounce wants the map: a built volume, a sun that
// bounces, shines and is shaded, and a relight to come.
static bool wanted(const voe_render_device *device)
{
	const struct voe_render_bounce_frame *begun = &device->bounce_frame;
	voe_render_bounce_lights lights;
	const struct voe_render_bounce_volume *volume = voe_render_bounce_volume_of(
		(voe_render_device *)device, device->bounce_target);

	VOE_BASE_DEBUG_ASSERT(device->bounce_begun,
			      "asking whether a bounce no begin placed wants a map");
	if (!volume->built || begun->sun_bounces == 0 ||
	    !(begun->sun.intensity > 0.0f) || begun->sun.unshaded != 0)
		return false;
	voe_render_bounce_begun_lights(device, &lights);
	return voe_render_bounce_probes_relight_needed(&volume->probes, &lights);
}

// Opens the cleared depth-only rendering onto `frame`'s map.
static void record_open(const struct voe_render_frame *frame)
{
	const VkPipelineStageFlags2 tests =
		VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
		VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
	const VkExtent2D extent = { TEXELS, TEXELS };
	VkRenderingAttachmentInfo depth = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = frame->bounce_shadow.map.view,
		.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = { .depthStencil = { .depth = VOE_RENDER_DEPTH_CLEAR } },
	};
	const VkRenderingInfo rendering = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea = { .extent = extent },
		.layerCount = 1,
		.pDepthAttachment = &depth,
	};
	const VkRect2D scissor = { .extent = extent };
	const VkViewport viewport = voe_render_frame_viewport(extent);

	VOE_BASE_DEBUG_ASSERT(depth.imageView != VK_NULL_HANDLE,
			      "opening a bounce sun map that was never built");
	// After the last pass onto it and the last relight that read it, the
	// previous view's this frame included.
	move(frame, VK_IMAGE_LAYOUT_UNDEFINED,
	     VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
	     tests | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_NONE,
	     tests,
	     VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
		     VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
	voe_render_vk.cmd_begin_rendering(frame->commands, &rendering);
	voe_render_vk.cmd_set_viewport(frame->commands, 0, 1, &viewport);
	voe_render_vk.cmd_set_scissor(frame->commands, 0, 1, &scissor);
}

bool voe_render_bounce_shadow_pass_begin(voe_render_device *device,
					 const voe_render_view *light,
					 bool *opened)
{
	struct voe_render_frame *frame;
	struct voe_render_frame_block block = { 0 };

	VOE_BASE_ASSERT(device != NULL && light != NULL && opened != NULL,
			"opening a bounce shadow pass on no device, with no light or nowhere to say so");
	VOE_BASE_ASSERT(device->recording,
			"opening a bounce shadow pass with no frame open");
	VOE_BASE_ASSERT(!device->pass_open,
			"opening a bounce shadow pass while a pass is already open — passes do not nest");
	// Without shaderOutputLayer a begin records nothing to assert on.
	VOE_BASE_ASSERT(device->bounce_begun || !device->output_layer,
			"opening a bounce shadow pass with no voe_render_bounce_begin this frame");
	frame = voe_render_frame_at(device, device->slot);
	VOE_BASE_ASSERT(!frame->bounce_shadow.drawn,
			"opening a second bounce shadow pass after one voe_render_bounce_begin — it opens once per begin");
	*opened = false;
	if (!device->bounce_begun || !wanted(device))
		return true;

	if (device->pass_count >= device->capacities.passes) {
		VOE_BASE_ERROR("render",
			       "this frame has already opened %u of %u passes, so a bounce shadow pass does not fit; `passes` is too small for what this frame draws",
			       device->pass_count, device->capacities.passes);
		return false;
	}

	block.camera = *light;
	block.depth_copy = VOE_RENDER_NO_DEPTH_COPY;
	block.bounce.grid = VOE_RENDER_NO_BOUNCE;
	block.bounce.spacing = VOE_RENDER_BOUNCE_SPACING;
	record_open(frame);
	device->pass_target = NULL;
	device->pass_extent = (VkExtent2D){ TEXELS, TEXELS };
	voe_render_pass_start(device, frame, &block, device->pipeline_shadow);
	device->pass_camera = true;
	device->pass_shadow = false;
	device->pass_bounce_shadow = true;
	frame->bounce_shadow.light =
		voe_math_float4x4_mul(light->projection, light->view);
	frame->bounce_shadow.drawn = true;
	*opened = true;
	return true;
}

void voe_render_bounce_shadow_end(voe_render_device *device)
{
	VOE_BASE_ASSERT(device != NULL && device->pass_bounce_shadow,
			"ending a bounce sun map outside the bounce shadow pass");
	move(voe_render_frame_at(device, device->slot),
	     VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
	     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	     VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
		     VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
	     VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
	     VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
	     VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
}
