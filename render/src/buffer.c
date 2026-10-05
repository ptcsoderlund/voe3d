// Buffers, and the staging upload that fills one. Two things live here: making a
// buffer with the memory under it, and copying bytes into a buffer the CPU
// cannot see. See device_internal.h for the contracts.
//
// THE UPLOAD PATTERN MATTERS MORE THAN THE CUBE DOES. A vertex buffer the GPU
// reads every frame belongs in device-local memory, which on a discrete card the
// CPU cannot write; so the bytes go into a host-visible staging buffer first and
// a queue copies them across, and every buffer is named for a capture tool
// (debug_names.c). Every later upload is this — a texture, a mesh out
// of a glTF file, anything at all — so it is one function with a contract rather
// than four lines repeated per caller.
//
// WHY IT IS NOT SIMPLY A HOST-VISIBLE VERTEX BUFFER. That works, it is one
// function shorter, and it is slower for the whole life of the program: the GPU
// would read every vertex across the bus on every draw. The staging copy pays
// once at startup instead. Where the trade genuinely goes the other way — data
// rewritten every frame — the right answer is a host-visible buffer that stays
// mapped, which is what the per-slot buffers in descriptors.c are and why they
// are built host-visible rather than through the upload below.
//
// ONE ALLOCATION PER BUFFER, AND THAT DOES NOT SCALE. Same note as target.c: a
// driver may refuse after a few thousand vkAllocateMemory calls. This engine
// makes a handful of buffers and one staging buffer at a time. The allocator is
// a card of its own.
//
// EVERY FAILURE HERE IS THE DRIVER REFUSING AND IS RETURNED, NOT ASSERTED. A
// card with no memory type that will do, an allocation that fails: the caller
// decides what that means, which is rule 13. A NULL device or a zero size is the
// caller's own bug and asserts.
#include "device_internal.h"

#include <base/assert.h>
#include <base/report.h>

#include <string.h>

bool voe_render_buffer_build(voe_render_device *device,
			     struct voe_render_buffer *buffer,
			     VkDeviceSize size, VkBufferUsageFlags usage,
			     VkMemoryPropertyFlags properties, const char *name)
{
	VkBufferCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};
	VkMemoryRequirements requirements;
	VkMemoryAllocateInfo allocate = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
	};
	uint32_t type;
	VkResult result;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "building a buffer without a device");
	VOE_BASE_DEBUG_ASSERT(buffer != NULL, "building a buffer into nothing");
	VOE_BASE_DEBUG_ASSERT(size > 0, "building a buffer of no bytes");

	*buffer = (struct voe_render_buffer){ 0 };

	result = voe_render_vk.create_buffer(device->device, &info, NULL,
					     &buffer->buffer);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkCreateBuffer failed for %llu bytes (VkResult %d)",
			       (unsigned long long)size, (int)result);
		buffer->buffer = VK_NULL_HANDLE;
		return false;
	}
	voe_render_debug_name(device, VK_OBJECT_TYPE_BUFFER,
			      (uint64_t)buffer->buffer, name);

	voe_render_vk.get_buffer_memory_requirements(device->device,
						     buffer->buffer,
						     &requirements);

	type = voe_render_memory_type(device, requirements.memoryTypeBits,
				      properties);
	if (type == UINT32_MAX) {
		VOE_BASE_ERROR("render",
			       "this graphics card offers no memory type a buffer with properties 0x%x can live in",
			       (unsigned)properties);
		return false;
	}

	allocate.allocationSize = requirements.size;
	allocate.memoryTypeIndex = type;
	result = voe_render_vk.allocate_memory(device->device, &allocate, NULL,
					       &buffer->memory);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkAllocateMemory failed for %llu bytes of buffer (VkResult %d)",
			       (unsigned long long)requirements.size, (int)result);
		buffer->memory = VK_NULL_HANDLE;
		return false;
	}

	result = voe_render_vk.bind_buffer_memory(device->device,
						  buffer->buffer,
						  buffer->memory, 0);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkBindBufferMemory failed (VkResult %d)",
			       (int)result);
		return false;
	}

	return true;
}

void voe_render_buffer_teardown(voe_render_device *device,
				struct voe_render_buffer *buffer)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "tearing down a buffer without a device");
	VOE_BASE_DEBUG_ASSERT(buffer != NULL, "tearing down nothing");

	if (device->device == VK_NULL_HANDLE)
		return;

	if (buffer->buffer != VK_NULL_HANDLE)
		voe_render_vk.destroy_buffer(device->device, buffer->buffer,
					     NULL);
	// Last, because the buffer was living in it.
	if (buffer->memory != VK_NULL_HANDLE)
		voe_render_vk.free_memory(device->device, buffer->memory, NULL);

	*buffer = (struct voe_render_buffer){ 0 };
}

// The staging buffer, filled from data. Split out so that the failure paths in
// the upload below have one thing to undo rather than three.
static bool fill_staging(voe_render_device *device,
			 struct voe_render_buffer *staging, const void *data,
			 VkDeviceSize size)
{
	// vkMapMemory hands back its pointer through a void **, which is
	// Vulkan's signature and not one this engine gets to choose.
	void *mapped = NULL;
	VkResult result;

	if (!voe_render_buffer_build(device, staging, size,
				     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				     "upload staging"))
		return false;

	result = voe_render_vk.map_memory(device->device, staging->memory, 0,
					  VK_WHOLE_SIZE, 0, &mapped);
	if (result != VK_SUCCESS || mapped == NULL) {
		VOE_BASE_ERROR("render",
			       "vkMapMemory failed on a staging buffer (VkResult %d)",
			       (int)result);
		return false;
	}

	memcpy(mapped, data, (size_t)size);

	// Coherent memory, so there is nothing to flush: the unmap is the whole
	// of making the write visible to the queue.
	voe_render_vk.unmap_memory(device->device, staging->memory);
	return true;
}

// The copy, recorded into a command buffer of its own and waited on. Its own,
// because this runs before the first frame and borrowing a frame slot's buffer
// would tie an upload to a slot for no reason; waited on, because the staging
// buffer is destroyed the moment this returns.
static bool copy_and_wait(voe_render_device *device,
			  const struct voe_render_buffer *destination,
			  const struct voe_render_buffer *staging,
			  VkDeviceSize offset, VkDeviceSize size)
{
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
	VkBufferCopy region = { .dstOffset = offset, .size = size };
	VkCommandBufferSubmitInfo submit_commands = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
	};
	VkSubmitInfo2 submit = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &submit_commands,
	};
	VkResult result;

	result = voe_render_vk.allocate_command_buffers(device->device,
							&allocate, &commands);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkAllocateCommandBuffers failed for an upload (VkResult %d)",
			       (int)result);
		return false;
	}

	voe_render_vk.begin_command_buffer(commands, &begin);
	voe_render_vk.cmd_copy_buffer(commands, staging->buffer,
				      destination->buffer, 1, &region);
	voe_render_vk.end_command_buffer(commands);

	submit_commands.commandBuffer = commands;
	result = voe_render_vk.queue_submit2(device->queue, 1, &submit,
					    VK_NULL_HANDLE);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkQueueSubmit2 failed for an upload (VkResult %d)",
			       (int)result);
		voe_render_vk.free_command_buffers(device->device, device->pool,
						   1, &commands);
		return false;
	}

	// No fence and no semaphore: idle is what makes the staging buffer and
	// this command buffer safe to destroy, it is the only thing this call
	// has to wait for, and nothing else is running yet. A fence here would
	// be a second object to create and destroy to say the same thing.
	//
	// NO BARRIER AFTER THE COPY EITHER, AND THAT IS NOT AN OMISSION. The
	// submit's completion — which idle waits for — makes the write available
	// to every later submit on this queue, and the first draw is in a later
	// submit. A barrier would be needed if the draw shared this command
	// buffer, and it does not.
	voe_render_vk.device_wait_idle(device->device);
	voe_render_vk.free_command_buffers(device->device, device->pool, 1,
					   &commands);
	return true;
}

bool voe_render_buffer_upload(voe_render_device *device,
			      const struct voe_render_buffer *buffer,
			      VkDeviceSize offset, const void *data,
			      VkDeviceSize size)
{
	struct voe_render_buffer staging = { 0 };
	bool uploaded;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "uploading without a device");
	VOE_BASE_DEBUG_ASSERT(buffer != NULL, "uploading into nothing");
	VOE_BASE_DEBUG_ASSERT(data != NULL, "uploading from nothing");
	VOE_BASE_DEBUG_ASSERT(size > 0, "uploading no bytes");
	VOE_BASE_DEBUG_ASSERT(device->pool != VK_NULL_HANDLE,
			      "uploading before there is a command pool");

	uploaded = fill_staging(device, &staging, data, size) &&
		   copy_and_wait(device, buffer, &staging, offset, size);

	// The staging buffer has done its whole job either way, and the copy
	// above has finished reading it.
	voe_render_buffer_teardown(device, &staging);
	return uploaded;
}
