// The sun's shadow maps (ADR-0258): per frame slot one D32 image of
// VOE_RENDER_SHADOW_CASCADES layers, a view of the whole array for the shader
// and one view per layer for a shadow pass to draw into, and the two barriers
// round such a pass. pass.c opens and closes shadow passes; this file owns the
// images and their layouts.
//
// LIFETIME: STARTUP TO SHUTDOWN. The maps' size is capacities.shadow_size, which
// no resize changes, so unlike target.c's images nothing here is rebuilt.
// open_device builds them after the frame objects, whose command pool settles them,
// and before the descriptors, which are what will name the array view.
//
// PER SLOT, because a frame in flight may still be sampling its slot's maps while
// the next frame draws into its own; one shared image would be drawn over while
// read. The slot's fence is what makes each slot's maps safe to redraw.
//
// ONE TEXEL WHEN THERE ARE NO SHADOWS. A device made with shadow_size nought
// still has a 1×1 map in every slot, so the descriptor the shader reads is always
// a valid one; it opens no shadow pass (pass.c asserts) and costs 16 bytes.
//
// THE RESTING LAYOUT IS SHADER_READ_ONLY_OPTIMAL. Startup settles every layer
// there; a shadow pass moves its one layer to the depth attachment layout and
// back. The move in comes from UNDEFINED, because the pass clears what was there.
#include "device_internal.h"

#include <base/assert.h>
#include <base/report.h>

// One image of every cascade with its memory, device-local only: drawn by the
// card and sampled by it, never touched by the CPU.
static bool build_image(voe_render_device *device,
			struct voe_render_shadow_map *map, uint32_t side)
{
	VkImageCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VOE_RENDER_DEPTH_FORMAT,
		.extent = { side, side, 1 },
		.mipLevels = 1,
		.arrayLayers = VOE_RENDER_SHADOW_CASCADES,
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
			       "vkCreateImage failed for a %ux%u shadow map (VkResult %d)",
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
			       "this graphics card offers no device-local memory a shadow map can live in");
		return false;
	}

	result = voe_render_vk.allocate_memory(device->device, &allocate, NULL,
					       &map->memory);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkAllocateMemory failed for %llu bytes of shadow map (VkResult %d)",
			       (unsigned long long)requirements.size, (int)result);
		map->memory = VK_NULL_HANDLE;
		return false;
	}

	result = voe_render_vk.bind_image_memory(device->device, map->image,
						 map->memory, 0);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkBindImageMemory failed on a shadow map (VkResult %d)",
			       (int)result);
		return false;
	}
	return true;
}

// The array view, then one 2D view per layer. `first` and `count` pick the
// layers; the array view is the one with every layer.
static bool build_view(voe_render_device *device, VkImage image,
		       VkImageViewType type, uint32_t first, uint32_t count,
		       VkImageView *out)
{
	VkImageViewCreateInfo view = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image,
		.viewType = type,
		.format = VOE_RENDER_DEPTH_FORMAT,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
			.levelCount = 1,
			.baseArrayLayer = first,
			.layerCount = count,
		},
	};
	VkResult result = voe_render_vk.create_image_view(device->device, &view,
							  NULL, out);

	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkCreateImageView failed on a shadow map (VkResult %d)",
			       (int)result);
		*out = VK_NULL_HANDLE;
		return false;
	}
	return true;
}

static bool build_map(voe_render_device *device,
		      struct voe_render_shadow_map *map, uint32_t side)
{
	if (!build_image(device, map, side))
		return false;
	if (!build_view(device, map->image, VK_IMAGE_VIEW_TYPE_2D_ARRAY, 0,
			VOE_RENDER_SHADOW_CASCADES, &map->array))
		return false;
	for (uint32_t i = 0; i < VOE_RENDER_SHADOW_CASCADES; i++) {
		if (!build_view(device, map->image, VK_IMAGE_VIEW_TYPE_2D, i, 1,
				&map->layers[i]))
			return false;
	}
	return true;
}

// One barrier over `count` layers of `image` from `from` to `to`, the stages and
// accesses either side taken from the layouts: a depth write on the attachment
// side, a fragment-stage read on the other.
static VkImageMemoryBarrier2 layout_barrier(VkImage image, uint32_t first,
					    uint32_t count, VkImageLayout from,
					    VkImageLayout to)
{
	const VkPipelineStageFlags2 tests =
		VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
		VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
	const bool to_attachment = to == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

	return (VkImageMemoryBarrier2){
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = to_attachment ? VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT :
						tests,
		.srcAccessMask = to_attachment ?
					 VK_ACCESS_2_NONE :
					 VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		.dstStageMask = to_attachment ? tests :
						VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
		.dstAccessMask = to_attachment ?
					 VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
						 VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT :
					 VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
		.oldLayout = from,
		.newLayout = to,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
			.levelCount = 1,
			.baseArrayLayer = first,
			.layerCount = count,
		},
	};
}

static void record_barrier(VkCommandBuffer commands,
			   const VkImageMemoryBarrier2 *barriers, uint32_t count)
{
	VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = count,
		.pImageMemoryBarriers = barriers,
	};

	voe_render_vk.cmd_pipeline_barrier2(commands, &dependency);
}

// Every slot's every layer out of UNDEFINED into the layout it rests in, in one
// submit the device waits for — startup's, so the wait costs nothing that
// matters, and the maps are then valid to read before any shadow pass has run.
static bool settle(voe_render_device *device)
{
	VkImageMemoryBarrier2 barriers[VOE_RENDER_FRAMES_IN_FLIGHT];
	VkCommandBufferAllocateInfo allocate = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = device->pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	VkCommandBuffer commands = VK_NULL_HANDLE;
	VkCommandBufferBeginInfo begin = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	VkCommandBufferSubmitInfo submit_commands = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
	};
	VkSubmitInfo2 submit = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &submit_commands,
	};
	VkResult result;

	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++)
		barriers[i] = layout_barrier(device->frames[i].shadow.image, 0,
					     VOE_RENDER_SHADOW_CASCADES,
					     VK_IMAGE_LAYOUT_UNDEFINED,
					     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	result = voe_render_vk.allocate_command_buffers(device->device,
							&allocate, &commands);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkAllocateCommandBuffers failed settling the shadow maps (VkResult %d)",
			       (int)result);
		return false;
	}

	voe_render_vk.begin_command_buffer(commands, &begin);
	record_barrier(commands, barriers, VOE_RENDER_FRAMES_IN_FLIGHT);
	voe_render_vk.end_command_buffer(commands);

	submit_commands.commandBuffer = commands;
	result = voe_render_vk.queue_submit2(device->queue, 1, &submit,
					     VK_NULL_HANDLE);
	if (result == VK_SUCCESS)
		voe_render_vk.device_wait_idle(device->device);
	else
		VOE_BASE_ERROR("render",
			       "vkQueueSubmit2 failed settling the shadow maps (VkResult %d)",
			       (int)result);

	voe_render_vk.free_command_buffers(device->device, device->pool, 1,
					   &commands);
	return result == VK_SUCCESS;
}

bool voe_render_shadow_startup(voe_render_device *device)
{
	uint32_t side;

	VOE_BASE_ASSERT(device != NULL, "making shadow maps on no device");
	VOE_BASE_ASSERT(device->pool != VK_NULL_HANDLE,
			"making shadow maps before the command pool that settles them");

	side = device->capacities.shadow_size > 0 ?
		       device->capacities.shadow_size : 1;

	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		if (!build_map(device, &device->frames[i].shadow, side))
			return false;
	}
	return settle(device);
}

void voe_render_shadow_shutdown(voe_render_device *device)
{
	VOE_BASE_ASSERT(device != NULL, "taking shadow maps from no device");

	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		struct voe_render_shadow_map *map = &device->frames[i].shadow;

		for (uint32_t j = 0; j < VOE_RENDER_SHADOW_CASCADES; j++) {
			if (map->layers[j] != VK_NULL_HANDLE)
				voe_render_vk.destroy_image_view(device->device,
								 map->layers[j], NULL);
		}
		if (map->array != VK_NULL_HANDLE)
			voe_render_vk.destroy_image_view(device->device,
							 map->array, NULL);
		if (map->image != VK_NULL_HANDLE)
			voe_render_vk.destroy_image(device->device, map->image,
						    NULL);
		// Last, because the image was living in it.
		if (map->memory != VK_NULL_HANDLE)
			voe_render_vk.free_memory(device->device, map->memory,
						  NULL);
		*map = (struct voe_render_shadow_map){ 0 };
	}
}

void voe_render_shadow_to_attachment(const struct voe_render_frame *frame,
				     uint32_t cascade)
{
	VkImageMemoryBarrier2 barrier;

	VOE_BASE_ASSERT(frame != NULL, "a shadow barrier on no frame slot");
	VOE_BASE_ASSERT(cascade < VOE_RENDER_SHADOW_CASCADES,
			"a shadow barrier on a cascade the map does not have");

	barrier = layout_barrier(frame->shadow.image, cascade, 1,
				 VK_IMAGE_LAYOUT_UNDEFINED,
				 VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
	record_barrier(frame->commands, &barrier, 1);
}

void voe_render_shadow_to_read(const struct voe_render_frame *frame,
			       uint32_t cascade)
{
	VkImageMemoryBarrier2 barrier;

	VOE_BASE_ASSERT(frame != NULL, "a shadow barrier on no frame slot");
	VOE_BASE_ASSERT(cascade < VOE_RENDER_SHADOW_CASCADES,
			"a shadow barrier on a cascade the map does not have");

	barrier = layout_barrier(frame->shadow.image, cascade, 1,
				 VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
				 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	record_barrier(frame->commands, &barrier, 1);
}
