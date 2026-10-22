// The growth of one static geometry pool (0444): a larger device-local buffer,
// what the old one holds copied across, and the old one given back. Called by
// create_geometry in geometry.c when a mesh finds no free range that holds it
// and no room past `used`. Ranges, the free list and every slot's offsets are
// counted in elements from the pool's start, so all of them stay as they are.
//
// GROWING IS SAFE HERE BECAUSE OF WHERE IT RUNS. A static create already waits
// for the card to go idle, under the device's guard, which keeps a frame open on
// another thread out until it has been submitted; the copy below is waited on
// the same way, so when the old buffer is destroyed no submitted frame still
// reads it. Draws bind the pool's buffer when they record (draw.c), so the next
// frame binds the new one and nothing has kept the old handle.
//
// THE PEAK IS OLD PLUS NEW, BRIEFLY. Both buffers exist from the new one's build
// to the old one's teardown, so a pool of N bytes doubling holds 3N for the
// length of one copy.
//
// IT DOUBLES. The new capacity is the larger of twice the old and what is
// needed, so a run of creates that each just overflow costs one copy per
// doubling and not one per mesh. When the card refuses that, it is asked once
// more for exactly what is needed, which a nearly full card may still hold.
//
// CONSTRAINTS. A count is 32 bits, so a need past UINT32_MAX elements is refused
// as that limit. A refused doubling prints the driver's line even when the retry
// then succeeds. The guard keeps out another thread's frame and not the caller's
// own: a grow inside a frame open on the calling thread frees a buffer that
// frame may already have bound, and deferring the old buffer's teardown to that
// frame slot's fence would lift it.
#include "device_internal.h"

#include <base/assert.h>
#include <base/report.h>

// One try at a pool buffer of `capacity` elements; on failure, whatever the
// build made is given back, so the caller has nothing to undo.
static bool build_grown(voe_render_device *device,
			struct voe_render_buffer *grown, uint32_t capacity,
			size_t element, VkBufferUsageFlags usage,
			const char *name)
{
	if (voe_render_buffer_build(device, grown,
				    (VkDeviceSize)capacity * element, usage,
				    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, name))
		return true;
	voe_render_buffer_teardown(device, grown);
	return false;
}

voe_base_error voe_render_geometry_grow(voe_render_device *device,
					struct voe_render_pool *pool,
					uint64_t needed, size_t element,
					VkBufferUsageFlags usage,
					const char *name)
{
	struct voe_render_buffer grown = { 0 };
	uint64_t doubled;
	uint32_t capacity;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "growing a pool on no device");
	VOE_BASE_DEBUG_ASSERT(pool != NULL, "growing no pool");
	VOE_BASE_DEBUG_ASSERT(needed > pool->capacity,
			      "growing a pool that already has the room");

	if (needed > UINT32_MAX) {
		VOE_BASE_ERROR("render",
			       "the most the engine takes in its %s is %u elements, and this mesh would need %llu",
			       name, UINT32_MAX, (unsigned long long)needed);
		return VOE_BASE_ERROR_UNSUPPORTED;
	}

	doubled = (uint64_t)pool->capacity * 2;
	capacity = (uint32_t)(doubled > UINT32_MAX ? UINT32_MAX
			      : doubled > needed  ? doubled
						  : needed);
	if (!build_grown(device, &grown, capacity, element, usage, name)) {
		if (capacity == needed)
			return VOE_BASE_ERROR_REFUSED;
		capacity = (uint32_t)needed;
		if (!build_grown(device, &grown, capacity, element, usage, name))
			return VOE_BASE_ERROR_REFUSED;
	}

	// An empty pool has nothing to copy, and is still waited on: a frame in
	// flight may have bound the old buffer before its last mesh went.
	if (pool->used == 0) {
		voe_render_vk.device_wait_idle(device->device);
	} else if (!voe_render_buffer_copy(device, &grown, &pool->buffer,
					   (VkDeviceSize)pool->used * element)) {
		voe_render_buffer_teardown(device, &grown);
		return VOE_BASE_ERROR_REFUSED;
	}

	voe_render_buffer_teardown(device, &pool->buffer);
	pool->buffer = grown;
	pool->capacity = capacity;
	return VOE_BASE_OK;
}
