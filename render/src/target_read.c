// The way back (ADR-0156): voe_render_target_read copies one target's finished
// picture into a caller's arena as RGBA8 with straight alpha. A host-visible
// staging buffer, a one-shot copy, an idle wait, and the channel swap and the
// divide by alpha on the way out.
//
// IT READS THE SLOT THE FRAME THAT ENDED LAST DREW INTO, which is the slot before
// device->slot. Both kinds of target go through it — the window's pair
// (target.c) and a caller's own (target_own.c) — because a picture saved and a
// picture shown being the same picture is the point rather than a coincidence.
#include "device_internal.h"

#include <base/assert.h>
#include <base/report.h>

// One channel, premultiplied by alpha, divided back out. Rounded rather than
// truncated, so that an opaque pixel comes back as the byte that went in, and
// clamped because a blend is allowed to leave a channel above its own alpha.
static uint8_t straight(unsigned value, unsigned alpha)
{
	unsigned out = (value * 255u + alpha / 2u) / alpha;

	return (uint8_t)(out > 255u ? 255u : out);
}

// One target's picture, in its own byte order and premultiplied, turned into
// what a caller was promised: RGBA8 with straight alpha.
//
// THE DIVIDE IS ON THE ENCODED BYTES AND THAT IS DELIBERATE. The format carries
// the sRGB curve, so these bytes are encoded; un-premultiplying them exactly
// would mean decoding, dividing and encoding again, and nothing in this engine
// draws a target whose alpha is anything but one — the clear colour is opaque,
// so every pixel of every picture taken so far divides by 255/255. Where a
// caller does compose a target out of transparent pixels, this is the cheap
// answer, and voe_render_target_read's header says which one it is.
//
// An alpha of nought has no colour to recover, so the pixel comes back as
// transparent black rather than as a division by nothing.
static void into_rgba(uint8_t *out, const uint8_t *in, size_t pixels, bool bgra)
{
	const size_t red = bgra ? 2 : 0;
	const size_t blue = bgra ? 0 : 2;

	for (size_t i = 0; i < pixels; i++) {
		const uint8_t *from = in + i * 4;
		uint8_t *to = out + i * 4;
		unsigned alpha = from[3];

		if (alpha == 0) {
			to[0] = 0;
			to[1] = 0;
			to[2] = 0;
			to[3] = 0;
			continue;
		}

		to[0] = straight(from[red], alpha);
		to[1] = straight(from[1], alpha);
		to[2] = straight(from[blue], alpha);
		to[3] = (uint8_t)alpha;
	}
}

// The copy itself: one command buffer, recorded, submitted and waited for. The
// same one-shot shape settle() in target_own.c has, and the same reason for
// waiting — the picture is wanted now, by a caller who has just ended a frame.
//
// `transition` IS FOR THE ONE CASE THE LAYOUTS DO NOT COVER. A window target
// that no frame has ended into is still UNDEFINED, which vkCmdCopyImageToBuffer
// does not accept as a source layout; a barrier out of UNDEFINED is always legal
// and discards only what was already undefined. Every other case is already in a
// layout the copy takes: TRANSFER_SRC_OPTIMAL for the window after a frame,
// GENERAL for a target of one's own always.
static bool copy_out(voe_render_device *device, VkImage image,
		     VkImageLayout layout, bool transition, VkExtent2D extent,
		     VkBuffer buffer)
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
	VkImageMemoryBarrier2 readable = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
		.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};
	VkDependencyInfo into_readable = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &readable,
	};
	VkBufferImageCopy region = {
		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.layerCount = 1,
		},
		.imageExtent = { extent.width, extent.height, 1 },
	};
	// What makes the copy visible to the map below. The idle wait after the
	// submit is documented to do the same job, and saying it here as well
	// costs one barrier and removes the question — the same pair
	// render/tests/offscreen.c uses.
	VkMemoryBarrier2 visible = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
		.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
		.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT,
	};
	VkDependencyInfo into_host = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.memoryBarrierCount = 1,
		.pMemoryBarriers = &visible,
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

	// The frame that drew this picture may still be on the card, and this
	// copy has nothing to order itself against it with — the frame's fence
	// belongs to frame.c. Idle is what says the picture is finished.
	voe_render_vk.device_wait_idle(device->device);

	result = voe_render_vk.allocate_command_buffers(device->device,
							&allocate, &commands);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkAllocateCommandBuffers failed reading a target back (VkResult %d)",
			       (int)result);
		return false;
	}

	voe_render_vk.begin_command_buffer(commands, &begin);
	if (transition)
		voe_render_vk.cmd_pipeline_barrier2(commands, &into_readable);
	voe_render_vk.cmd_copy_image_to_buffer(commands, image, layout, buffer,
					       1, &region);
	voe_render_vk.cmd_pipeline_barrier2(commands, &into_host);
	voe_render_vk.end_command_buffer(commands);

	submit_commands.commandBuffer = commands;
	result = voe_render_vk.queue_submit2(device->queue, 1, &submit,
					     VK_NULL_HANDLE);
	if (result == VK_SUCCESS)
		voe_render_vk.device_wait_idle(device->device);
	else
		VOE_BASE_ERROR("render",
			       "vkQueueSubmit2 failed reading a target back (VkResult %d)",
			       (int)result);

	voe_render_vk.free_command_buffers(device->device, device->pool, 1,
					   &commands);
	return result == VK_SUCCESS;
}

bool voe_render_target_read(voe_render_device *device, voe_render_target target,
			    voe_base_arena *arena, voe_render_picture *out,
			    voe_base_error *error)
{
	const struct voe_render_allocated_image *colour;
	struct voe_render_buffer staging = { 0 };
	struct voe_render_target_slot *own;
	VkExtent2D extent;
	VkImageLayout layout;
	bool transition = false;
	uint32_t slot;
	VkDeviceSize bytes;
	void *mapped = NULL;
	VkResult result;

	VOE_BASE_ASSERT(device != NULL, "reading a target back off no device");
	VOE_BASE_ASSERT(arena != NULL, "reading a target back into no arena");
	VOE_BASE_ASSERT(out != NULL,
			"reading a target back with nowhere to describe it");
	VOE_BASE_ASSERT(!device->recording,
			"reading a target back inside an open frame — it waits for the card, so it belongs between frames beside voe_render_target_create");

	// The frame that ended last drew into the slot before the one the next
	// frame will use: voe_render_frame_end spends a slot the moment its
	// submit lands.
	slot = (device->slot + VOE_RENDER_FRAMES_IN_FLIGHT - 1) %
	       VOE_RENDER_FRAMES_IN_FLIGHT;

	if (target.index == VOE_RENDER_TARGET_WINDOW.index) {
		colour = &device->frames[slot].target.colour;
		extent = device->resolution;
		layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		transition = !device->frame_ended;
	} else {
		own = voe_render_target_at(device, target);
		VOE_BASE_ASSERT(own != NULL,
				"reading back a target id that names no target");
		colour = &own->images[slot].colour;
		extent = own->extent;
		// A target of one's own rests in GENERAL for its whole life,
		// drawn into or not; see settle() in target_own.c.
		layout = VK_IMAGE_LAYOUT_GENERAL;
	}

	if (extent.width == 0 || extent.height == 0 ||
	    colour->image == VK_NULL_HANDLE) {
		VOE_BASE_ERROR("render",
			       "this target has no pixels to read — a window with no area has no picture");
		goto refused;
	}

	bytes = (VkDeviceSize)extent.width * extent.height * 4;

	// Coherent as well as visible, so that reading it after the idle wait
	// needs no invalidate call.
	if (!voe_render_buffer_build(device, &staging, bytes,
				     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
		goto refused;

	if (!copy_out(device, colour->image, layout, transition, extent,
		      staging.buffer))
		goto refused_with_buffer;

	result = voe_render_vk.map_memory(device->device, staging.memory, 0,
					  VK_WHOLE_SIZE, 0, &mapped);
	if (result != VK_SUCCESS || mapped == NULL) {
		VOE_BASE_ERROR("render",
			       "vkMapMemory failed on the buffer a target was read into (VkResult %d)",
			       (int)result);
		goto refused_with_buffer;
	}

	out->width = extent.width;
	out->height = extent.height;
	out->pixels = voe_base_arena_push(arena, (size_t)bytes);
	into_rgba(out->pixels, mapped, (size_t)extent.width * extent.height,
		  device->format.format == VK_FORMAT_B8G8R8A8_SRGB);

	voe_render_vk.unmap_memory(device->device, staging.memory);
	voe_render_buffer_teardown(device, &staging);
	return true;

refused_with_buffer:
	voe_render_buffer_teardown(device, &staging);
refused:
	if (error != NULL)
		*error = VOE_BASE_ERROR_REFUSED;
	return false;
}
