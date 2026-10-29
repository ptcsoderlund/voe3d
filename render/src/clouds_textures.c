// The cloud field bound for the shaders (ADR-0492 point 4): the 64³ R8_UNORM
// detail volume of binding 13 and the 256² R8_UNORM weather map of binding 14,
// sampled and transfer-dst, filled from clouds_noise.c through one staging
// buffer (buffer.c), resting in SHADER_READ_ONLY_OPTIMAL; and the one linear
// sampler, repeating on every axis, both are read through. Every image, view
// and the sampler is named for a capture.
//
//     voe_render_clouds_textures_build(device);    // at open, after the pool
//     voe_render_descriptors_build(device);        // names all three
//     voe_render_clouds_textures_teardown(device); // at close
//
// WHY AT OPEN. The bytes are the same on every run and every machine, so there
// is nothing to wait for: made once before the first frame, the upload costs
// one idle at startup, and every slot's set names a valid image from the
// moment it exists. The noise is written straight into the mapped staging
// buffer, so no copy of the bytes is held on the heap.
//
// THE COST. 64³ + 256² = 327 680 bytes of device-local memory for the device's
// life, and the same again in host-visible staging for the moment of the upload.
//
// CONSTRAINTS. One allocation per image, as target.c makes. One level each, no
// mip chain. A failed build leaves what it made for _teardown, which close
// calls on every path.
#include "clouds_noise.h"
#include "device_internal.h"

#include <base/assert.h>
#include <base/report.h>

#include <stdint.h>

#define DETAIL_BYTES                                                           \
	((VkDeviceSize)VOE_RENDER_CLOUDS_DETAIL_SIZE *                         \
	 VOE_RENDER_CLOUDS_DETAIL_SIZE * VOE_RENDER_CLOUDS_DETAIL_SIZE)
#define WEATHER_BYTES                                                          \
	((VkDeviceSize)VOE_RENDER_CLOUDS_WEATHER_SIZE *                        \
	 VOE_RENDER_CLOUDS_WEATHER_SIZE)

// One R8_UNORM image of `extent`, 3D when its depth is above one, its memory
// and its view, each named `what`; false with a message.
static bool build_image(voe_render_device *device,
			struct voe_render_allocated_image *out,
			VkExtent3D extent, const char *what)
{
	const bool volume = extent.depth > 1;
	VkImageCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = volume ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D,
		.format = VK_FORMAT_R8_UNORM,
		.extent = extent,
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_SAMPLED_BIT |
			 VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};
	VkImageViewCreateInfo view = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.viewType = volume ? VK_IMAGE_VIEW_TYPE_3D : VK_IMAGE_VIEW_TYPE_2D,
		.format = VK_FORMAT_R8_UNORM,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};
	VkMemoryRequirements requirements;
	VkMemoryAllocateInfo allocate = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
	};
	VkResult result;

	VOE_BASE_DEBUG_ASSERT(device != NULL && out != NULL && what != NULL,
			      "building a cloud image with no device or nowhere");
	result = voe_render_vk.create_image(device->device, &info, NULL,
					    &out->image);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkCreateImage failed for the %s (VkResult %d)",
			       what, (int)result);
		out->image = VK_NULL_HANDLE;
		return false;
	}
	voe_render_debug_name(device, VK_OBJECT_TYPE_IMAGE, (uint64_t)out->image,
			      what);
	voe_render_vk.get_image_memory_requirements(device->device, out->image,
						    &requirements);
	allocate.allocationSize = requirements.size;
	allocate.memoryTypeIndex = voe_render_memory_type(
		device, requirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (allocate.memoryTypeIndex == UINT32_MAX) {
		VOE_BASE_ERROR("render",
			       "this graphics card offers no device-local memory the %s can live in",
			       what);
		return false;
	}
	result = voe_render_vk.allocate_memory(device->device, &allocate, NULL,
					       &out->memory);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkAllocateMemory failed for %llu bytes of the %s (VkResult %d)",
			       (unsigned long long)requirements.size, what,
			       (int)result);
		out->memory = VK_NULL_HANDLE;
		return false;
	}
	result = voe_render_vk.bind_image_memory(device->device, out->image,
						 out->memory, 0);
	view.image = out->image;
	if (result == VK_SUCCESS)
		result = voe_render_vk.create_image_view(device->device, &view,
							 NULL, &out->view);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "binding or viewing the %s failed (VkResult %d)",
			       what, (int)result);
		out->view = VK_NULL_HANDLE;
		return false;
	}
	voe_render_debug_name(device, VK_OBJECT_TYPE_IMAGE_VIEW,
			      (uint64_t)out->view, what);
	return true;
}

// `image` into TRANSFER_DST for the copy, or after it out to where the
// fragment stage samples it; the stages and accesses are the copy's own.
static VkImageMemoryBarrier2 settle(VkImage image, bool before_copy)
{
	VOE_BASE_DEBUG_ASSERT(image != VK_NULL_HANDLE, "settling no cloud image");
	return (VkImageMemoryBarrier2){
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = before_copy ? VK_PIPELINE_STAGE_2_NONE :
					      VK_PIPELINE_STAGE_2_COPY_BIT,
		.srcAccessMask = before_copy ? 0 : VK_ACCESS_2_TRANSFER_WRITE_BIT,
		.dstStageMask = before_copy ?
					VK_PIPELINE_STAGE_2_COPY_BIT :
					VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
		.dstAccessMask = before_copy ?
					 VK_ACCESS_2_TRANSFER_WRITE_BIT :
					 VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
		.oldLayout = before_copy ? VK_IMAGE_LAYOUT_UNDEFINED :
					   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.newLayout = before_copy ?
				     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL :
				     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};
}

// Both images' barriers in, the two copies out of `staging` (the detail at its
// start, the weather after it), the barriers out, in one command buffer
// submitted and waited on: copy_into_image's shape in texture.c, for its
// reasons. False with a message.
static bool upload(voe_render_device *device,
		   const struct voe_render_buffer *staging)
{
	const VkImage images[2] = { device->clouds_detail.image,
				    device->clouds_weather.image };
	const VkBufferImageCopy regions[2] = {
		{
			.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
			.imageExtent = { VOE_RENDER_CLOUDS_DETAIL_SIZE,
					 VOE_RENDER_CLOUDS_DETAIL_SIZE,
					 VOE_RENDER_CLOUDS_DETAIL_SIZE },
		},
		{
			.bufferOffset = DETAIL_BYTES,
			.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
			.imageExtent = { VOE_RENDER_CLOUDS_WEATHER_SIZE,
					 VOE_RENDER_CLOUDS_WEATHER_SIZE, 1 },
		},
	};
	VkImageMemoryBarrier2 barriers[2];
	const VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 2,
		.pImageMemoryBarriers = barriers,
	};
	const VkCommandBufferAllocateInfo allocate = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = device->pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	const VkCommandBufferBeginInfo begin = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	VkCommandBuffer commands = VK_NULL_HANDLE;
	VkCommandBufferSubmitInfo submit_commands = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
	};
	const VkSubmitInfo2 submit = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &submit_commands,
	};
	VkResult result;

	VOE_BASE_DEBUG_ASSERT(staging != NULL && staging->buffer != VK_NULL_HANDLE,
			      "uploading the cloud field from no staging buffer");
	result = voe_render_vk.allocate_command_buffers(device->device,
							&allocate, &commands);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkAllocateCommandBuffers failed for the cloud field (VkResult %d)",
			       (int)result);
		return false;
	}
	result = voe_render_vk.begin_command_buffer(commands, &begin);
	if (result == VK_SUCCESS) {
		for (uint32_t i = 0; i < 2; i++)
			barriers[i] = settle(images[i], true);
		voe_render_vk.cmd_pipeline_barrier2(commands, &dependency);
		for (uint32_t i = 0; i < 2; i++)
			voe_render_vk.cmd_copy_buffer_to_image(
				commands, staging->buffer, images[i],
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
				&regions[i]);
		for (uint32_t i = 0; i < 2; i++)
			barriers[i] = settle(images[i], false);
		voe_render_vk.cmd_pipeline_barrier2(commands, &dependency);
		result = voe_render_vk.end_command_buffer(commands);
	}
	submit_commands.commandBuffer = commands;
	if (result == VK_SUCCESS)
		result = voe_render_vk.queue_submit2(device->queue, 1, &submit,
						     VK_NULL_HANDLE);
	if (result == VK_SUCCESS)
		result = voe_render_vk.device_wait_idle(device->device);
	if (result != VK_SUCCESS)
		VOE_BASE_ERROR("render",
			       "uploading the cloud field failed (VkResult %d)",
			       (int)result);
	voe_render_vk.free_command_buffers(device->device, device->pool, 1,
					   &commands);
	return result == VK_SUCCESS;
}

// The one linear sampler, repeating on every axis because both fields tile.
static bool build_sampler(voe_render_device *device)
{
	const VkSamplerCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
	};
	VkResult result;

	VOE_BASE_DEBUG_ASSERT(device->clouds_sampler == VK_NULL_HANDLE,
			      "building the clouds sampler twice");
	result = voe_render_vk.create_sampler(device->device, &info, NULL,
					      &device->clouds_sampler);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkCreateSampler failed for the cloud field (VkResult %d)",
			       (int)result);
		device->clouds_sampler = VK_NULL_HANDLE;
		return false;
	}
	voe_render_debug_name(device, VK_OBJECT_TYPE_SAMPLER,
			      (uint64_t)device->clouds_sampler, "clouds sampler");
	return true;
}

bool voe_render_clouds_textures_build(voe_render_device *device)
{
	struct voe_render_buffer staging = { 0 };
	void *mapped = NULL;
	VkResult result;
	bool built;

	VOE_BASE_DEBUG_ASSERT(device != NULL && device->pool != VK_NULL_HANDLE,
			      "building the cloud field before the command pool");
	VOE_BASE_DEBUG_ASSERT(device->clouds_detail.image == VK_NULL_HANDLE,
			      "building the cloud field twice");
	if (!build_image(device, &device->clouds_detail,
			 (VkExtent3D){ VOE_RENDER_CLOUDS_DETAIL_SIZE,
				       VOE_RENDER_CLOUDS_DETAIL_SIZE,
				       VOE_RENDER_CLOUDS_DETAIL_SIZE },
			 "clouds detail") ||
	    !build_image(device, &device->clouds_weather,
			 (VkExtent3D){ VOE_RENDER_CLOUDS_WEATHER_SIZE,
				       VOE_RENDER_CLOUDS_WEATHER_SIZE, 1 },
			 "clouds weather") ||
	    !build_sampler(device))
		return false;
	if (!voe_render_buffer_build(device, &staging,
				     DETAIL_BYTES + WEATHER_BYTES,
				     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				     "clouds staging")) {
		voe_render_buffer_teardown(device, &staging);
		return false;
	}
	// vkMapMemory's void ** is Vulkan's signature, not this engine's.
	result = voe_render_vk.map_memory(device->device, staging.memory, 0,
					  VK_WHOLE_SIZE, 0, &mapped);
	built = result == VK_SUCCESS && mapped != NULL;
	if (built) {
		voe_render_clouds_detail(mapped);
		voe_render_clouds_weather((uint8_t *)mapped + DETAIL_BYTES);
		// Coherent: the unmap is the whole of making the bytes visible.
		voe_render_vk.unmap_memory(device->device, staging.memory);
		built = upload(device, &staging);
	} else {
		VOE_BASE_ERROR("render",
			       "vkMapMemory failed on the cloud field's staging (VkResult %d)",
			       (int)result);
	}
	voe_render_buffer_teardown(device, &staging);
	return built;
}

void voe_render_clouds_textures_teardown(voe_render_device *device)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "tearing down no device's clouds");
	if (device->device == VK_NULL_HANDLE)
		return;
	voe_render_target_image_teardown(device, &device->clouds_detail);
	voe_render_target_image_teardown(device, &device->clouds_weather);
	if (device->clouds_sampler != VK_NULL_HANDLE) {
		voe_render_vk.destroy_sampler(device->device,
					      device->clouds_sampler, NULL);
		device->clouds_sampler = VK_NULL_HANDLE;
	}
}
