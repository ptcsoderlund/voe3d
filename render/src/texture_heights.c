// A landscape's heights (ADR-0396 points 3 and 5): an R32F texture of metres the
// vertex stage reads with texel loads, made once and written a rectangle at a
// time inside a frame. See render/include/render/device.h for what a caller sees.
//
// CREATE IS texture.c's UPLOAD WITH ANOTHER FORMAT. voe_render_texture_fill
// builds the image, copies the floats in and idles; the slot is then claimed
// SHARP (nearest, one level), marked `heights`, and the sets rewritten, all
// under the device's guard as voe_render_texture_create is.
//
// A WRITE IS RECORDED, NOT SUBMITTED. Each frame slot has its own host-visible
// staging of capacities.heights_texels floats, mapped for its life; a write
// copies into it after what this frame already wrote and records into the
// frame's own command buffer, before any pass: the image leaves
// SHADER_READ_ONLY for TRANSFER_DST after earlier vertex reads, takes the copy,
// and returns before this frame's vertex reads. Reads by a frame still in flight
// were submitted earlier, so the queue's order covers them; nothing waits. The
// staging is the slot's for the reason the transient pools are: the fence at the
// frame's top says the card is done with it, and frame.c then empties it.
//
// CONSTRAINTS. No guard on a write: it runs inside a frame, on the frame's
// thread. A write over the slot's remaining room is refused whole, never cut, so
// the caller carries the rectangle to the next frame (0396's carried rest).
#include "device_internal.h"

#include <base/assert.h>
#include <base/report.h>

#include <string.h>

// Vulkan's guaranteed 2D side, and a 2049-texel side (2048 cells) sits inside it.
#define HEIGHTS_SIDE_MAX 4096

bool voe_render_texture_heights_startup(voe_render_device *device)
{
	const VkDeviceSize size =
		(VkDeviceSize)device->capacities.heights_texels * sizeof(float);

	VOE_BASE_DEBUG_ASSERT(device != NULL, "starting heights on no device");
	if (size == 0)
		return true;

	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		struct voe_render_frame *frame = &device->frames[i];
		VkResult result;

		if (!voe_render_buffer_build(device, &frame->heights, size,
					     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
						     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					     "heights staging"))
			return false;
		result = voe_render_vk.map_memory(device->device,
						  frame->heights.memory, 0,
						  VK_WHOLE_SIZE, 0,
						  &frame->heights_mapped);
		if (result != VK_SUCCESS || frame->heights_mapped == NULL) {
			VOE_BASE_ERROR("render",
				       "vkMapMemory failed on a heights staging buffer (VkResult %d)",
				       (int)result);
			frame->heights_mapped = NULL;
			return false;
		}
	}
	VOE_BASE_DEBUG_ASSERT(device->frames[0].heights_mapped != NULL,
			      "heights staging built and not mapped");
	return true;
}

void voe_render_texture_heights_shutdown(voe_render_device *device)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "shutting down heights on no device");

	// Freeing the memory unmaps it.
	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		voe_render_buffer_teardown(device, &device->frames[i].heights);
		device->frames[i].heights_mapped = NULL;
		device->frames[i].heights_used = 0;
	}
	VOE_BASE_DEBUG_ASSERT(device->frames[0].heights.buffer == VK_NULL_HANDLE,
			      "heights staging left after shutdown");
}

static bool create_heights(voe_render_device *device, uint32_t width,
			   uint32_t height, const float *heights,
			   voe_render_texture *out)
{
	const uint32_t index = voe_render_target_free_texture(device, 0);
	struct voe_render_texture_slot *slot;

	if (index == 0) {
		VOE_BASE_ERROR("render",
			       "all %d texture slots are taken, and a heights texture needs one",
			       VOE_RENDER_MAX_TEXTURES);
		return false;
	}
	slot = &device->textures[index];
	if (!voe_render_texture_fill(device, slot, VK_FORMAT_R32_SFLOAT, width,
				     height, 1, heights,
				     (VkDeviceSize)width * height *
					     sizeof(float)))
		return false;

	// Claimed once filled, as create_texture does. SHARP names a sampler
	// the descriptor must have; the vertex stage loads and never samples.
	slot->sampling = VOE_RENDER_SAMPLING_SHARP;
	slot->heights = true;
	slot->width = width;
	slot->height = height;
	slot->generation++;
	slot->live = true;
	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++)
		voe_render_texture_write_descriptors(device, i);

	out->index = index;
	out->generation = slot->generation;
	return true;
}

bool voe_render_texture_create_heights(voe_render_device *device,
				       uint32_t width, uint32_t height,
				       const float *heights,
				       voe_render_texture *out,
				       voe_base_error *error)
{
	bool created;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "creating heights with no device");
	VOE_BASE_DEBUG_ASSERT(heights != NULL && out != NULL,
			      "creating heights from or into nothing");
	VOE_BASE_DEBUG_ASSERT(width > 0 && height > 0,
			      "creating heights with no texels");

	if (width > HEIGHTS_SIDE_MAX || height > HEIGHTS_SIDE_MAX) {
		VOE_BASE_ERROR("render",
			       "a %ux%u heights texture is past the %d a side every card offers",
			       width, height, HEIGHTS_SIDE_MAX);
		if (error != NULL)
			*error = VOE_BASE_ERROR_REFUSED;
		return false;
	}

	voe_render_device_guard_take(device);
	created = create_heights(device, width, height, heights, out);
	voe_render_device_guard_give(device);
	if (!created && error != NULL)
		*error = VOE_BASE_ERROR_REFUSED;
	return created;
}

// One heights image's layout move with the vertex stage on the far side.
static void heights_barrier(VkCommandBuffer commands, VkImage image,
			    bool to_transfer)
{
	VkImageMemoryBarrier2 barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = to_transfer ? VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT :
					      VK_PIPELINE_STAGE_2_COPY_BIT,
		.srcAccessMask = to_transfer ? 0 : VK_ACCESS_2_TRANSFER_WRITE_BIT,
		.dstStageMask = to_transfer ? VK_PIPELINE_STAGE_2_COPY_BIT :
					      VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
		.dstAccessMask = to_transfer ? VK_ACCESS_2_TRANSFER_WRITE_BIT :
					       VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
		.oldLayout = to_transfer ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL :
					   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.newLayout = to_transfer ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL :
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
	VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &barrier,
	};

	VOE_BASE_DEBUG_ASSERT(image != VK_NULL_HANDLE, "a barrier on no image");
	voe_render_vk.cmd_pipeline_barrier2(commands, &dependency);
}

bool voe_render_texture_write_heights(voe_render_device *device,
				      voe_render_texture texture, uint32_t x,
				      uint32_t y, uint32_t width,
				      uint32_t height, const float *values,
				      voe_base_error *error)
{
	const struct voe_render_texture_slot *slot;
	struct voe_render_frame *frame;
	const uint32_t count = width * height;
	VkBufferImageCopy region = {
		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.layerCount = 1,
		},
		.imageOffset = { (int32_t)x, (int32_t)y, 0 },
		.imageExtent = { width, height, 1 },
	};

	VOE_BASE_DEBUG_ASSERT(device != NULL && values != NULL,
			      "writing heights on no device or from nothing");
	VOE_BASE_ASSERT(device->recording,
			"writing heights with no frame open — the staging is the slot voe_render_frame_begin picks");
	VOE_BASE_ASSERT(!device->pass_open && device->pass_count == 0,
			"writing heights after a pass began — every pass of the frame must read them");
	VOE_BASE_ASSERT(texture.index < VOE_RENDER_MAX_TEXTURES,
			"writing heights into no texture");
	slot = &device->textures[texture.index];
	VOE_BASE_ASSERT(slot->live && slot->generation == texture.generation &&
				slot->heights,
			"writing heights into a texture that is not a live heights one");
	VOE_BASE_ASSERT(width > 0 && height > 0 && x <= slot->width - width &&
				y <= slot->height - height && width <= slot->width &&
				height <= slot->height,
			"writing a heights rectangle past the texture");

	frame = voe_render_frame_open(device);
	if (count > device->capacities.heights_texels - frame->heights_used) {
		VOE_BASE_ERROR("render",
			       "no room this frame for %u heights texels — %u of %u spent; heights_texels is too small for what this frame writes",
			       count, frame->heights_used,
			       device->capacities.heights_texels);
		if (error != NULL)
			*error = VOE_BASE_ERROR_REFUSED;
		return false;
	}

	// Coherent memory, so the submit makes the write visible.
	memcpy((float *)frame->heights_mapped + frame->heights_used, values,
	       (size_t)count * sizeof(float));
	region.bufferOffset = (VkDeviceSize)frame->heights_used * sizeof(float);

	heights_barrier(frame->commands, slot->image, true);
	voe_render_vk.cmd_copy_buffer_to_image(
		frame->commands, frame->heights.buffer, slot->image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	heights_barrier(frame->commands, slot->image, false);

	frame->heights_used += count;
	VOE_BASE_DEBUG_ASSERT(frame->heights_used <=
				      device->capacities.heights_texels,
			      "heights staging overrun");
	return true;
}
