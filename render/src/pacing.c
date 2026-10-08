// Timing and pacing: what the card's clock is worth and whether the surface
// offers MAILBOX, both asked once at startup; the present mode a caller sets and
// reads; and the GPU time the last measured frame took.
//
// This was cut from device.c for length, not because it stands apart: open_device
// (device.c) still calls voe_render_timing_learn and then
// voe_render_present_modes_learn, after the logical device and its format and
// before the frame objects, because the timing decides whether there are query
// pools to make. Neither learn can fail; startup.h declares both.
//
// The set and the two reads are render/device.h's. Setting a mode only marks the
// swapchain for a rebuild; swapchain.c is where the mode comes into force.
#include "device_internal.h"
#include "startup.h"

#include <render/device.h>

#include <base/assert.h>

#include <stddef.h>

// What the card's clock is worth, asked once. Two numbers and they are separate
// questions: the period is the card's, out of its limits, and the valid bits are
// the queue family's — a card can write timestamps and the family this device
// took can still be one that does not.
//
// A CARD THAT CANNOT TIME IS NOT A FAILURE AND IS NOT WORTH A MESSAGE AT
// STARTUP. Every desktop card measured writes timestamps; the ones that do not
// are compute-only queues and virtualised drivers, and the answer there is a
// frame loop that reports CPU time and says the GPU number is missing. So this
// returns nothing: it sets three fields and the caller carries on either way.
void voe_render_timing_learn(voe_render_device *device, voe_base_arena *arena)
{
	VkPhysicalDeviceProperties properties;
	VkQueueFamilyProperties *families;
	uint32_t count = 0;

	voe_render_vk.get_physical_device_properties(device->physical,
						     &properties);

	// Zero means the card cannot do it at all, and the specification says so
	// in exactly those words.
	if (properties.limits.timestampPeriod == 0.0f)
		return;

	voe_render_vk.get_queue_family_properties(device->physical, &count, NULL);
	if (count == 0 || device->queue_family >= count)
		return;

	families = voe_base_arena_push(arena, (size_t)count * sizeof(*families));
	voe_render_vk.get_queue_family_properties(device->physical, &count,
						  families);

	if (families[device->queue_family].timestampValidBits == 0)
		return;

	device->timestamp_period = properties.limits.timestampPeriod;
	device->timestamp_valid_bits =
		families[device->queue_family].timestampValidBits;
	device->timestamps = true;
}

// Whether this surface offers MAILBOX, asked once for the same reason the format
// is: it is a property of a physical device and a surface, and a resize changes
// neither. A headless device has no surface and presents nothing.
//
// FALSE IS AN ORDINARY ANSWER AND NOT A FAILURE. MAILBOX is optional in the
// specification; only FIFO is required of everyone. So a query that will not
// answer is read as "no mailbox here" and the device stays on the mode every
// driver has to support.
void voe_render_present_modes_learn(voe_render_device *device,
				    voe_base_arena *arena)
{
	VkPresentModeKHR *modes;
	uint32_t count = 0;

	if (device->headless)
		return;

	if (voe_render_vk.get_surface_present_modes(device->physical,
						    device->surface, &count,
						    NULL) != VK_SUCCESS ||
	    count == 0)
		return;

	modes = voe_base_arena_push(arena, (size_t)count * sizeof(*modes));
	if (voe_render_vk.get_surface_present_modes(device->physical,
						    device->surface, &count,
						    modes) != VK_SUCCESS)
		return;

	for (uint32_t i = 0; i < count; i++) {
		if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			device->mailbox_offered = true;
			return;
		}
	}
}

void voe_render_present_set(voe_render_device *device, voe_render_present mode)
{
	VOE_BASE_ASSERT(device != NULL, "asking no device to present differently");
	VOE_BASE_ASSERT(mode == VOE_RENDER_PRESENT_FIFO ||
				mode == VOE_RENDER_PRESENT_MAILBOX,
			"asking for a present mode that is not one of the two");

	device->present_wanted = mode;

	// The swapchain is what carries the mode, so changing it is building
	// another one — and that happens at the top of a frame, where every
	// other rebuild happens, because the images the presentation engine is
	// still reading are not ours to destroy from here.
	if (device->present_wanted != device->present_in_force)
		device->rebuild = true;
}

voe_render_present voe_render_present_get(const voe_render_device *device)
{
	VOE_BASE_ASSERT(device != NULL, "asking no device how it presents");

	return device->present_in_force;
}

bool voe_render_frame_gpu_time(const voe_render_device *device, double *seconds)
{
	VOE_BASE_ASSERT(device != NULL, "asking no device what the card's clock said");
	VOE_BASE_ASSERT(seconds != NULL,
			"asking for a GPU time with nowhere to put it");

	if (!device->gpu_measured)
		return false;

	*seconds = device->gpu_seconds;
	return true;
}
