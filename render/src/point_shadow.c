// The point lights' shadow maps (ADR-0325 point 1): per frame slot one D32 2D-array
// image of 6 × VOE_RENDER_POINT_SHADOWS layers, a sampled view of every layer for
// binding 9 and an attachment view of the same layers for the layered pass that
// draws them, and voe_render_point_shadows_ready. binding 9 reads them through
// shadow.c's comparison sampler; nothing here makes a sampler of its own.
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
// THE RESTING LAYOUT IS SHADER_READ_ONLY_OPTIMAL, settled at startup.
#include "device_internal.h"

#include <base/assert.h>
#include <base/report.h>

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

static bool build_map(voe_render_device *device,
		      struct voe_render_point_shadow_map *map, uint32_t side)
{
	if (!build_image(device, map, side))
		return false;
	map->sampled = build_view(device, map->image);
	if (map->sampled == VK_NULL_HANDLE)
		return false;
	map->attachment = build_view(device, map->image);
	return map->attachment != VK_NULL_HANDLE;
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
		if (!build_map(device, &device->frames[i].point_shadow, side))
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
