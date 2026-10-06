// The shading records: one buffer of them the fragment stage reads by index, and
// one slot each on this side so a stale id can be refused.
//
// A RECORD IS WRITTEN ONCE AND NEVER AGAIN, WHICH IS WHY ONE BUFFER SERVES EVERY
// FRAME SLOT. The per-object records change every frame and are therefore per
// slot; these do not change at all after they are made, so a second copy would
// be a second copy of something nobody writes.
//
// IT IS HOST-VISIBLE AND WRITTEN DIRECTLY RATHER THAN STAGED. A record is eighty
// bytes and there are as many of them as a scene has materials, so the staging
// buffer and the queue submit an upload would cost buy nothing measurable; what
// they would cost is a second code path that only startup uses.
//
// CREATING ONE WAITS FOR THE GPU TO GO IDLE, WHICH MAKES IT A STARTUP OPERATION.
// The write below goes into memory a frame in flight may be reading, and this is
// the honest way to say "not while anything is drawing". Materials that arrive
// while the engine is running want a per-slot buffer or a copy queue, and that
// is the card that needs them.
//
// A RECORD IS FREED BY voe_render_shading_destroy (0278), which gives its slot
// back with the generation bumped; a create takes the lowest free slot. The
// same idle wait makes destroying one a startup operation too.
//
// THE TEXTURE IDS IN A RECORD ARE INDEX HALVES AND ARE NOT CHECKED. An id that
// names no live texture samples the one-pixel white default rather than failing,
// because a material referring to a picture that did not load is a thing to see
// rather than a thing to stop for — and because the generation half, which is
// what would catch it, deliberately never reaches the GPU.
#include "device_internal.h"

#include <base/assert.h>
#include <base/report.h>

#include <stdlib.h>
#include <string.h>

bool voe_render_shading_startup(voe_render_device *device)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "starting shading on no device");
	VOE_BASE_DEBUG_ASSERT(device->capacities.shadings > 0,
			      "a device with room for no shading records");

	device->shading_slots = calloc(device->capacities.shadings,
				       sizeof(*device->shading_slots));
	VOE_BASE_ASSERT(device->shading_slots != NULL,
			"out of memory making room for a device's shading records");

	if (!voe_render_buffer_build(
		    device, &device->shadings,
		    (VkDeviceSize)device->capacities.shadings *
			    sizeof(voe_render_shading_values),
		    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		    "shading records"))
		return false;

	if (voe_render_vk.map_memory(device->device, device->shadings.memory, 0,
				     VK_WHOLE_SIZE, 0,
				     &device->shadings_mapped) != VK_SUCCESS ||
	    device->shadings_mapped == NULL) {
		VOE_BASE_ERROR("render",
			       "vkMapMemory failed on the shading buffer");
		device->shadings_mapped = NULL;
		return false;
	}

	// Zeroed, so a record nothing has written is black with no textures
	// rather than whatever the driver left in the memory. Nothing should
	// draw with one, and a caller that does sees black instead of noise.
	memset(device->shadings_mapped, 0,
	       (size_t)device->capacities.shadings *
		       sizeof(voe_render_shading_values));
	return true;
}

void voe_render_shading_shutdown(voe_render_device *device)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "shutting shading down on no device");

	if (device->device != VK_NULL_HANDLE &&
	    device->shadings_mapped != NULL) {
		voe_render_vk.unmap_memory(device->device,
					   device->shadings.memory);
		device->shadings_mapped = NULL;
	}
	voe_render_buffer_teardown(device, &device->shadings);

	free(device->shading_slots);
	device->shading_slots = NULL;
}

static bool create_shading(voe_render_device *device,
			   voe_render_shading_values values,
			   voe_render_shading *out, voe_base_error *error)
{
	struct voe_render_shading_slot *slot = NULL;
	uint32_t index = 0;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "making a shading record on no device");
	VOE_BASE_DEBUG_ASSERT(out != NULL, "making a shading record into nothing");
	VOE_BASE_DEBUG_ASSERT(device->shadings_mapped != NULL,
			      "making a shading record before the buffer exists");

	for (uint32_t i = 0; i < device->capacities.shadings; i++) {
		if (!device->shading_slots[i].live) {
			index = i;
			slot = &device->shading_slots[i];
			break;
		}
	}
	if (slot == NULL) {
		VOE_BASE_ERROR("render",
			       "all %u shading records are taken; the device was made with room for that many",
			       device->capacities.shadings);
		if (error != NULL)
			*error = VOE_BASE_ERROR_REFUSED;
		return false;
	}

	// Not while a frame is reading it — see the header.
	voe_render_vk.device_wait_idle(device->device);

	memcpy((unsigned char *)device->shadings_mapped +
		       (size_t)index * sizeof(values),
	       &values, sizeof(values));

	slot->generation++;
	slot->live = true;

	out->index = index;
	out->generation = slot->generation;
	return true;
}

// Under the device's guard (ADR-0370), as is the destroy.
bool voe_render_shading_create(voe_render_device *device,
			       voe_render_shading_values values,
			       voe_render_shading *out, voe_base_error *error)
{
	bool created;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "making a shading record on no device");

	voe_render_device_guard_take(device);
	created = create_shading(device, values, out, error);
	voe_render_device_guard_give(device);
	return created;
}

// The row's bytes are left as they are: nothing may draw with the old id, and
// the next create of this slot overwrites them. The wait is the create's, for
// the same reason: that overwrite may not land while a frame reads the row.
static bool destroy_shading(voe_render_device *device,
			    voe_render_shading shading)
{
	struct voe_render_shading_slot *slot;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "destroying a shading record on no device");

	if (device->shading_slots == NULL ||
	    shading.index >= device->capacities.shadings)
		return false;
	slot = &device->shading_slots[shading.index];
	if (!slot->live || slot->generation != shading.generation)
		return false;

	voe_render_vk.device_wait_idle(device->device);

	slot->generation++;
	slot->live = false;
	return true;
}

bool voe_render_shading_destroy(voe_render_device *device,
				voe_render_shading shading)
{
	bool destroyed;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "destroying a shading record on no device");

	voe_render_device_guard_take(device);
	destroyed = destroy_shading(device, shading);
	voe_render_device_guard_give(device);
	return destroyed;
}
