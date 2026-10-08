// The bounce shadow pass (ADR-0329 points 2 and 3, 0357 point 4): each bouncing
// sun's depth map the relight shadows by, drawn as a shadow pass is but onto a
// map of its own. voe_render_bounce_shadow_pass_begin opens it for one sun after
// the begun target's capture passes; voe_render_pass_end calls
// voe_render_bounce_shadow_end once its rendering has ended. Draws in it take the
// shadow pass's path (draw.c).
//
// WHY A MAP OF ITS OWN. The cascades follow the view, so a relight made while
// looking away found the room behind the camera outside them and lit it
// unshadowed. 3d fits this map to the probe volume, which stands on the eye, so
// it answers the same whichever way the camera faces.
//
// THE MAP. Per frame slot a D32 image of VOE_RENDER_DIRECTIONAL_LIGHTS layers
// VOE_RENDER_BOUNCE_SHADOW_TEXELS square, a depth attachment and sampled (and a
// transfer source, so a test may read it), with a 2D-array view of every layer
// for the relight and one view per layer to draw into. Built at startup beside
// the capture scratch on a device with shaderOutputLayer, none without, and every
// layer settled into SHADER_READ_ONLY_OPTIMAL, where it rests, so a descriptor
// naming it is valid on a frame that drew none. Layer i is the begin's bouncing
// sun i: the first sun's 0, a further sun's its place among those that bounce
// + 1 (device->bounce_sun_layers, the order the relight reads them in). The slot
// keeps each layer's view × projection and whether the current begin drew it:
// that flag is per begin (ADR-0330 point 1), cleared by each
// voe_render_bounce_begin, so every view the editor shows draws the maps for its
// own volume, and a second open of one sun after one begin asserts.
//
// THE BEGIN opens only when a probe of the begun volume changed or the lights
// did, never for probes fading in alone, and the named sun bounces, shines and
// is shaded: its layer from UNDEFINED into its
// attachment layout, depth cleared to the far plane, the shadow pipeline, the
// viewport and scissor at the map's side, `light` as the pass's camera, the begun
// volume's spacing in its block. Its barrier out of UNDEFINED waits on the
// fragment tests and on the compute stage as well: the slot has one map, so a
// second view's pass follows the previous view's relight reading it this frame,
// and overwrites it only after. THE END moves that layer alone back to
// SHADER_READ_ONLY_OPTIMAL before a compute read.
//
// COST. 16 MiB of depth a frame slot, 32 MiB over two, made whether or not a
// further sun ever bounces (0357: growing it would be a second lazy image). At
// most one pass per bouncing sun per begun target (ADR-0330 point 3) and one
// object per caster drawn into each; nothing on a settled frame.
#include "frame_internal.h"

#include <base/assert.h>
#include <base/report.h>

#include <stdio.h>

#define TEXELS VOE_RENDER_BOUNCE_SHADOW_TEXELS
#define SUNS VOE_RENDER_DIRECTIONAL_LIGHTS

// A view of `count` layers of `image` from `first`; VK_NULL_HANDLE in `out` and a
// line when the card refuses it.
static bool build_view(voe_render_device *device, VkImage image,
		       VkImageViewType type, uint32_t first, uint32_t count,
		       VkImageView *out)
{
	const VkImageViewCreateInfo view = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image,
		.viewType = type,
		.format = VOE_RENDER_DEPTH_FORMAT,
		.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
				      .levelCount = 1,
				      .baseArrayLayer = first,
				      .layerCount = count },
	};
	const VkResult result =
		voe_render_vk.create_image_view(device->device, &view, NULL, out);

	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkCreateImageView failed on a bounce sun map (VkResult %d)",
			       (int)result);
		*out = VK_NULL_HANDLE;
		return false;
	}
	return true;
}

// One slot's map: the image of every sun's layer in device-local memory, its
// array view and a view per layer. False with a line; what was made is left for
// the shutdown to free. Each is named for frame slot `slot`.
static bool build_map(voe_render_device *device,
		      struct voe_render_bounce_shadow *shadow, uint32_t slot)
{
	char name[64];
	const VkImageCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VOE_RENDER_DEPTH_FORMAT,
		.extent = { TEXELS, TEXELS, 1 },
		.mipLevels = 1,
		.arrayLayers = SUNS,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
			 VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};
	VkMemoryRequirements requirements;
	VkMemoryAllocateInfo allocate = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
	};
	struct voe_render_allocated_image *map = &shadow->map;
	VkResult result =
		voe_render_vk.create_image(device->device, &info, NULL, &map->image);

	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkCreateImage failed for a bounce sun map (VkResult %d)",
			       (int)result);
		map->image = VK_NULL_HANDLE;
		return false;
	}
	snprintf(name, sizeof name, "bounce sun map slot %u", slot);
	voe_render_debug_name(device, VK_OBJECT_TYPE_IMAGE,
			      (uint64_t)map->image, name);
	voe_render_vk.get_image_memory_requirements(device->device, map->image,
						    &requirements);
	allocate.allocationSize = requirements.size;
	allocate.memoryTypeIndex = voe_render_memory_type(
		device, requirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (allocate.memoryTypeIndex == UINT32_MAX) {
		VOE_BASE_ERROR("render",
			       "this graphics card offers no device-local memory a bounce sun map can live in");
		return false;
	}
	result = voe_render_vk.allocate_memory(device->device, &allocate, NULL,
					       &map->memory);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkAllocateMemory failed for %llu bytes of bounce sun map (VkResult %d)",
			       (unsigned long long)requirements.size, (int)result);
		map->memory = VK_NULL_HANDLE;
		return false;
	}
	result = voe_render_vk.bind_image_memory(device->device, map->image,
						 map->memory, 0);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkBindImageMemory failed on a bounce sun map (VkResult %d)",
			       (int)result);
		return false;
	}
	if (!build_view(device, map->image, VK_IMAGE_VIEW_TYPE_2D_ARRAY, 0, SUNS,
			&map->view))
		return false;
	voe_render_debug_name(device, VK_OBJECT_TYPE_IMAGE_VIEW,
			      (uint64_t)map->view, name);
	for (uint32_t i = 0; i < SUNS; i++) {
		if (!build_view(device, map->image, VK_IMAGE_VIEW_TYPE_2D, i, 1,
				&shadow->layers[i]))
			return false;
		snprintf(name, sizeof name, "bounce sun map slot %u layer %u",
			 slot, i);
		voe_render_debug_name(device, VK_OBJECT_TYPE_IMAGE_VIEW,
				      (uint64_t)shadow->layers[i], name);
	}
	return true;
}

bool voe_render_bounce_shadow_startup(voe_render_device *device)
{
	VkImageMemoryBarrier2 settles[VOE_RENDER_FRAMES_IN_FLIGHT];

	VOE_BASE_ASSERT(device != NULL, "making bounce sun maps on no device");
	if (!device->output_layer)
		return true;
	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		struct voe_render_bounce_shadow *shadow =
			&device->frames[i].bounce_shadow;

		if (!build_map(device, shadow, i))
			return false;
		settles[i] = voe_render_target_settle_copy(shadow->map.image);
		settles[i].subresourceRange.layerCount = SUNS;
	}
	return voe_render_target_settle(device, settles,
					VOE_RENDER_FRAMES_IN_FLIGHT);
}

void voe_render_bounce_shadow_shutdown(voe_render_device *device)
{
	VOE_BASE_ASSERT(device != NULL, "taking bounce sun maps from no device");
	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		struct voe_render_bounce_shadow *shadow =
			&device->frames[i].bounce_shadow;

		for (uint32_t j = 0; j < SUNS; j++) {
			if (shadow->layers[j] != VK_NULL_HANDLE)
				voe_render_vk.destroy_image_view(
					device->device, shadow->layers[j], NULL);
			shadow->layers[j] = VK_NULL_HANDLE;
		}
		voe_render_target_image_teardown(device, &shadow->map);
	}
}

// Layer `layer` of the map between `from` and `to`, after `src` and before `dst`
// stages with their accesses.
static void move(const struct voe_render_frame *frame, uint32_t layer,
		 VkImageLayout from, VkImageLayout to, VkPipelineStageFlags2 src,
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
				      .baseArrayLayer = layer,
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

// Whether any probe of `probes` is marked changed.
static bool any_changed(const voe_render_bounce_probes *probes)
{
	VOE_BASE_DEBUG_ASSERT(probes != NULL, "no probes to ask");
	for (uint32_t i = 0; i < VOE_RENDER_BOUNCE_PROBES_TOTAL / 32; i++)
		if (probes->changed[i] != 0)
			return true;
	return false;
}

// Whether this frame's begun bounce wants layer `layer`'s map: a built volume,
// its sun bouncing, shining and shaded, and a probe changed or the lights did;
// a begin with only fading probes settles alone (ADR-0389 point 4).
static bool wanted(const voe_render_device *device, uint32_t layer)
{
	const struct voe_render_bounce_frame *begun = &device->bounce_frame;
	const voe_render_light *sun = layer == 0 ?
					      &begun->sun :
					      &device->bounce_suns[layer - 1].light;
	const uint32_t bounces = layer == 0 ? begun->sun_bounces :
					      device->bounce_suns[layer - 1].bounces;
	voe_render_bounce_lights lights;
	const struct voe_render_bounce_volume *volume = voe_render_bounce_volume_of(
		(voe_render_device *)device, device->bounce_target,
		device->bounce_volume);

	VOE_BASE_DEBUG_ASSERT(device->bounce_begun,
			      "asking whether a bounce no begin placed wants a map");
	if (!volume->built || bounces == 0 || !(sun->intensity > 0.0f) ||
	    sun->unshaded != 0)
		return false;
	if (any_changed(&volume->probes))
		return true;
	voe_render_bounce_begun_lights(device, &lights);
	return voe_render_bounce_probes_lights_changed(&volume->probes, &lights);
}

// Opens the cleared depth-only rendering onto layer `layer` of `frame`'s map.
static void record_open(const struct voe_render_frame *frame, uint32_t layer)
{
	const VkPipelineStageFlags2 tests =
		VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
		VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
	const VkExtent2D extent = { TEXELS, TEXELS };
	VkRenderingAttachmentInfo depth = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = frame->bounce_shadow.layers[layer],
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
	move(frame, layer, VK_IMAGE_LAYOUT_UNDEFINED,
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
					 uint32_t sun,
					 const voe_render_view *light,
					 bool *opened)
{
	struct voe_render_frame *frame;
	struct voe_render_frame_block block = { 0 };
	uint32_t layer;

	VOE_BASE_ASSERT(device != NULL && light != NULL && opened != NULL,
			"opening a bounce shadow pass on no device, with no light or nowhere to say so");
	VOE_BASE_ASSERT(device->recording,
			"opening a bounce shadow pass with no frame open");
	VOE_BASE_ASSERT(!device->pass_open,
			"opening a bounce shadow pass while a pass is already open — passes do not nest");
	*opened = false;
	// Before the begin's assert: a failed prepare also refused the begin.
	if (!voe_render_device_ready(device))
		return false;
	// Without shaderOutputLayer a begin records nothing to assert on.
	VOE_BASE_ASSERT(device->bounce_begun || !device->output_layer,
			"opening a bounce shadow pass with no voe_render_bounce_begin this frame");
	if (!device->bounce_begun)
		return true;
	VOE_BASE_ASSERT(sun < device->bounce_sun_count,
			"opening a bounce shadow pass for a sun at or past 1 + the begin's further suns");
	layer = device->bounce_sun_layers[sun];
	if (layer == VOE_RENDER_NO_SUN_LAYER)
		return true;
	frame = voe_render_frame_at(device, device->slot);
	VOE_BASE_ASSERT(!frame->bounce_shadow.drawn[layer],
			"opening a second bounce shadow pass for one sun after one voe_render_bounce_begin — it opens once per sun per begin");
	if (!wanted(device, layer))
		return true;

	if (device->pass_count >= device->capacities.passes) {
		VOE_BASE_ERROR("render",
			       "this frame has already opened %u of %u passes, so a bounce shadow pass does not fit; `passes` is too small for what this frame draws",
			       device->pass_count, device->capacities.passes);
		return false;
	}

	block.camera = *light;
	block.depth_copy = VOE_RENDER_NO_DEPTH_COPY;
	for (uint32_t v = 0; v < VOE_RENDER_BOUNCE_VOLUMES; v++)
		block.bounce[v].grid = VOE_RENDER_NO_BOUNCE;
	block.bounce[0].spacing = device->bounce_frame.spacing;
	record_open(frame, layer);
	device->pass_target = NULL;
	device->pass_extent = (VkExtent2D){ TEXELS, TEXELS };
	voe_render_pass_start(device, frame, &block, device->pipeline_shadow,
			      "bounce sun shadow");
	device->pass_camera = true;
	device->pass_shadow = false;
	device->pass_bounce_shadow = true;
	device->pass_bounce_layer = layer;
	frame->bounce_shadow.light[layer] =
		voe_math_float4x4_mul(light->projection, light->view);
	frame->bounce_shadow.drawn[layer] = true;
	*opened = true;
	return true;
}

void voe_render_bounce_shadow_end(voe_render_device *device)
{
	VOE_BASE_ASSERT(device != NULL && device->pass_bounce_shadow,
			"ending a bounce sun map outside the bounce shadow pass");
	move(voe_render_frame_at(device, device->slot), device->pass_bounce_layer,
	     VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
	     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	     VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
		     VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
	     VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
	     VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
	     VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
}
