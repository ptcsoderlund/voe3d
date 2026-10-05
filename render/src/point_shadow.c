// The point lights' shadow maps (ADR-0325 point 1): per frame slot one D32 2D-array
// image of 6 × VOE_RENDER_POINT_SHADOWS layers, a sampled view of every layer for
// binding 9 and an attachment view of the same layers for the layered pass that
// draws them, voe_render_point_shadows_ready, and that pass (ADR-0325 point 2).
// binding 9 reads them through shadow.c's comparison sampler; nothing here
// makes a sampler of its own.
//
// THE PASS is opened here through pass.c's start and light copy and closed by
// its _pass_end, which calls voe_render_point_shadow_to_read. Its camera block
// is a shadow pass's with no view and no probe volume, its spacing nought: the
// vertex stage reads only the lights, written into the pass's region at index
// slot − 1, so layer / 6 finds its own.
//
// THE LAYER OF SLOT s FACE f. Slot s is 1-based (0 is no shadow), faces are in
// the order +X −X +Y −Y +Z −Z, f from 0: layer 6(s − 1) + f.
//
// LIFETIME: STARTUP TO SHUTDOWN, like shadow.c's. The side is
// device->point_shadow_size, which device.c settled at create (nought on a card
// without shaderOutputLayer) and no resize changes. open_device builds them after
// the sun's maps and before the descriptors, which name the sampled view.
//
// PER SLOT, for shadow.c's reason: a frame in flight may still be sampling its
// slot's maps while the next frame draws into its own.
//
// ONE TEXEL WHEN THERE ARE NONE. A device whose side is nought still has a 1×1
// image of every layer in every slot, so binding 9 always names a valid view of
// the layer count the shader is written for; it costs 384 bytes a slot, and
// voe_render_point_shadows_ready says false.
//
// THE RESTING LAYOUT IS SHADER_READ_ONLY_OPTIMAL, settled at startup; the pass
// moves every layer to the depth attachment layout, from UNDEFINED because it
// clears them all, and back.
#include "frame_internal.h"

#include <base/assert.h>
#include <base/report.h>

#include <stdio.h>

#define LAYERS (6 * VOE_RENDER_POINT_SHADOWS)

// The image and the memory under it, device-local: drawn and sampled by the
// card, never touched by the CPU.
static bool build_image(voe_render_device *device,
			struct voe_render_point_shadow_map *map, uint32_t side)
{
	VkImageCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VOE_RENDER_DEPTH_FORMAT,
		.extent = { side, side, 1 },
		.mipLevels = 1,
		.arrayLayers = LAYERS,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
			 VK_IMAGE_USAGE_SAMPLED_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};
	VkMemoryRequirements requirements;
	VkMemoryAllocateInfo allocate = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
	};
	VkResult result;

	result = voe_render_vk.create_image(device->device, &info, NULL,
					    &map->image);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkCreateImage failed for a %ux%u point shadow map (VkResult %d)",
			       side, side, (int)result);
		map->image = VK_NULL_HANDLE;
		return false;
	}

	voe_render_vk.get_image_memory_requirements(device->device, map->image,
						    &requirements);
	allocate.allocationSize = requirements.size;
	allocate.memoryTypeIndex =
		voe_render_memory_type(device, requirements.memoryTypeBits,
				       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (allocate.memoryTypeIndex == UINT32_MAX) {
		VOE_BASE_ERROR("render",
			       "this graphics card offers no device-local memory a point shadow map can live in");
		return false;
	}

	result = voe_render_vk.allocate_memory(device->device, &allocate, NULL,
					       &map->memory);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkAllocateMemory failed for %llu bytes of point shadow map (VkResult %d)",
			       (unsigned long long)requirements.size, (int)result);
		map->memory = VK_NULL_HANDLE;
		return false;
	}

	result = voe_render_vk.bind_image_memory(device->device, map->image,
						 map->memory, 0);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkBindImageMemory failed on a point shadow map (VkResult %d)",
			       (int)result);
		return false;
	}
	return true;
}

// A 2D-array view of every layer: the sampled one and the attachment one are
// both this, two handles so each use names its own.
static VkImageView build_view(voe_render_device *device, VkImage image)
{
	VkImageViewCreateInfo view = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
		.format = VOE_RENDER_DEPTH_FORMAT,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
			.levelCount = 1,
			.layerCount = LAYERS,
		},
	};
	VkImageView out = VK_NULL_HANDLE;
	VkResult result = voe_render_vk.create_image_view(device->device, &view,
							  NULL, &out);

	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkCreateImageView failed on a point shadow map (VkResult %d)",
			       (int)result);
		return VK_NULL_HANDLE;
	}
	return out;
}

// The image and both views, named for frame slot `slot`.
static bool build_map(voe_render_device *device,
		      struct voe_render_point_shadow_map *map, uint32_t side,
		      uint32_t slot)
{
	char name[64];

	if (!build_image(device, map, side))
		return false;
	snprintf(name, sizeof name, "point shadow map slot %u", slot);
	voe_render_debug_name(device, VK_OBJECT_TYPE_IMAGE,
			      (uint64_t)map->image, name);
	map->sampled = build_view(device, map->image);
	if (map->sampled == VK_NULL_HANDLE)
		return false;
	snprintf(name, sizeof name, "point shadow map slot %u sampled", slot);
	voe_render_debug_name(device, VK_OBJECT_TYPE_IMAGE_VIEW,
			      (uint64_t)map->sampled, name);
	map->attachment = build_view(device, map->image);
	if (map->attachment == VK_NULL_HANDLE)
		return false;
	snprintf(name, sizeof name, "point shadow map slot %u attachment", slot);
	voe_render_debug_name(device, VK_OBJECT_TYPE_IMAGE_VIEW,
			      (uint64_t)map->attachment, name);
	return true;
}

// Every layer of one slot's image out of UNDEFINED into where the shader reads it.
static VkImageMemoryBarrier2 settle_barrier(VkImage image)
{
	return (VkImageMemoryBarrier2){
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_NONE,
		.srcAccessMask = VK_ACCESS_2_NONE,
		.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
		.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
			.levelCount = 1,
			.layerCount = LAYERS,
		},
	};
}

bool voe_render_point_shadow_startup(voe_render_device *device)
{
	VkImageMemoryBarrier2 barriers[VOE_RENDER_FRAMES_IN_FLIGHT];
	uint32_t side;

	VOE_BASE_ASSERT(device != NULL, "making point shadow maps on no device");
	VOE_BASE_ASSERT(device->pool != VK_NULL_HANDLE,
			"making point shadow maps before the command pool that settles them");

	side = device->point_shadow_size > 0 ? device->point_shadow_size : 1;
	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		if (!build_map(device, &device->frames[i].point_shadow, side, i))
			return false;
		barriers[i] = settle_barrier(device->frames[i].point_shadow.image);
	}
	return voe_render_target_settle(device, barriers,
					VOE_RENDER_FRAMES_IN_FLIGHT);
}

void voe_render_point_shadow_shutdown(voe_render_device *device)
{
	VOE_BASE_ASSERT(device != NULL, "taking point shadow maps from no device");

	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		struct voe_render_point_shadow_map *map =
			&device->frames[i].point_shadow;

		if (map->attachment != VK_NULL_HANDLE)
			voe_render_vk.destroy_image_view(device->device,
							 map->attachment, NULL);
		if (map->sampled != VK_NULL_HANDLE)
			voe_render_vk.destroy_image_view(device->device,
							 map->sampled, NULL);
		if (map->image != VK_NULL_HANDLE)
			voe_render_vk.destroy_image(device->device, map->image,
						    NULL);
		// Last, because the image was living in it.
		if (map->memory != VK_NULL_HANDLE)
			voe_render_vk.free_memory(device->device, map->memory,
						  NULL);
		*map = (struct voe_render_point_shadow_map){ 0 };
	}
	VOE_BASE_DEBUG_ASSERT(device->frames[0].point_shadow.image == VK_NULL_HANDLE,
			      "a point shadow map outlived its shutdown");
}

bool voe_render_point_shadows_ready(const voe_render_device *device)
{
	VOE_BASE_ASSERT(device != NULL, "asking no device about point shadows");
	VOE_BASE_DEBUG_ASSERT(device->point_shadow_size == 0 || device->output_layer,
			      "point shadow maps on a device that cannot draw them");

	return device->point_shadow_size > 0;
}

// Every layer of `frame`'s maps into the depth attachment layout (`to_attachment`)
// or back to where the fragment stage samples them, recorded.
static void record_layout(const struct voe_render_frame *frame,
			  bool to_attachment)
{
	const VkPipelineStageFlags2 tests =
		VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
		VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
	const VkAccessFlags2 write = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	VkImageMemoryBarrier2 barrier = settle_barrier(frame->point_shadow.image);
	VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &barrier,
	};

	if (to_attachment) {
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		barrier.dstStageMask = tests;
		barrier.dstAccessMask =
			VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | write;
		barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	} else {
		barrier.srcStageMask = tests;
		barrier.srcAccessMask = write;
		barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	}
	VOE_BASE_DEBUG_ASSERT(barrier.oldLayout != barrier.newLayout,
			      "a point shadow barrier that moves nothing");
	voe_render_vk.cmd_pipeline_barrier2(frame->commands, &dependency);
}

void voe_render_point_shadow_to_read(const struct voe_render_frame *frame)
{
	VOE_BASE_ASSERT(frame != NULL, "a point shadow barrier on no frame slot");
	record_layout(frame, false);
}

// The slotted lights of `lights` into the device's pass record, by slot − 1,
// and into pass `region` of the slot's light buffer at the same index.
static void place_casters(voe_render_device *device,
			  const struct voe_render_frame *frame, uint32_t region,
			  const voe_render_point_lights *lights)
{
	VOE_BASE_ASSERT(lights->count <= VOE_RENDER_POINT_LIGHTS,
			"a point-shadow pass with more lights than VOE_RENDER_POINT_LIGHTS");
	VOE_BASE_ASSERT(lights->lights != NULL || lights->count == 0,
			"a point-shadow pass with a light count and no lights");
	device->pass_slots = 0;
	for (uint32_t i = 0; i < lights->count; i++) {
		const voe_render_point_light *light = &lights->lights[i];
		const uint32_t slot = light->shadow;

		if (slot == 0)
			continue;
		VOE_BASE_ASSERT(slot <= VOE_RENDER_POINT_SHADOWS,
				"a point light whose shadow slot is past VOE_RENDER_POINT_SHADOWS");
		VOE_BASE_ASSERT((device->pass_slots & (1u << (slot - 1))) == 0,
				"two point lights in one pass naming the same shadow slot");
		device->pass_slots |= 1u << (slot - 1);
		device->pass_casters[slot - 1] = *light;
		voe_render_pass_copy_lights(frame, region, slot - 1, light, 1);
	}
	VOE_BASE_ASSERT(device->pass_slots != 0,
			"opening a point-shadow pass with no light holding a shadow slot");
}

bool voe_render_point_shadow_pass_begin(voe_render_device *device,
					const voe_render_point_lights *lights)
{
	struct voe_render_frame *frame;
	struct voe_render_frame_block block = { 0 };
	VkExtent2D extent;
	VkRenderingAttachmentInfo depth = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = { .depthStencil = { .depth = VOE_RENDER_DEPTH_CLEAR } },
	};
	VkRenderingInfo rendering = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.layerCount = LAYERS,
		.pDepthAttachment = &depth,
	};
	VkViewport viewport;
	VkRect2D scissor = { 0 };

	VOE_BASE_ASSERT(device != NULL, "opening a point-shadow pass on no device");
	VOE_BASE_ASSERT(lights != NULL, "opening a point-shadow pass with no lights");
	VOE_BASE_ASSERT(device->recording,
			"opening a point-shadow pass with no frame open");
	VOE_BASE_ASSERT(!device->pass_open,
			"opening a point-shadow pass while a pass is already open — passes do not nest");
	VOE_BASE_ASSERT(voe_render_point_shadows_ready(device),
			"opening a point-shadow pass on a device whose point shadows are not ready");

	if (device->pass_count >= device->capacities.passes) {
		VOE_BASE_ERROR("render",
			       "this frame has already opened %u of %u passes, so a point-shadow pass does not fit; `passes` is too small for what this frame draws",
			       device->pass_count, device->capacities.passes);
		return false;
	}
	if (!voe_render_device_ready(device))
		return false;

	frame = voe_render_frame_at(device, device->slot);
	place_casters(device, frame, device->pass_count, lights);
	block.depth_copy = VOE_RENDER_NO_DEPTH_COPY;
	block.bounce.grid = VOE_RENDER_NO_BOUNCE;
	block.region = device->pass_count;
	block.lights = VOE_RENDER_POINT_SHADOWS;
	extent = (VkExtent2D){ device->point_shadow_size,
			       device->point_shadow_size };
	depth.imageView = frame->point_shadow.attachment;
	rendering.renderArea.extent = extent;
	scissor.extent = extent;
	viewport = voe_render_frame_viewport(extent);

	record_layout(frame, true);
	voe_render_vk.cmd_begin_rendering(frame->commands, &rendering);
	voe_render_vk.cmd_set_viewport(frame->commands, 0, 1, &viewport);
	voe_render_vk.cmd_set_scissor(frame->commands, 0, 1, &scissor);

	device->pass_target = NULL;
	device->pass_extent = extent;
	voe_render_pass_start(device, frame, &block,
			      device->pipeline_point_shadow, "point shadows");
	device->pass_camera = true;
	device->pass_shadow = false;
	device->pass_point_shadow = true;
	return true;
}
