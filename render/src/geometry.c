// The two geometry pools and the ranges into them. One vertex pool, one index
// pool, both device-local, both appended to and never freed, and one slot per
// range so that a voe_render_geometry id can be refused when it names nothing.
//
// A MESH IS A RANGE AND NOT A BUFFER, AND THAT IS THE WHOLE POINT OF THIS FILE.
// A frame binds the two pools once and then draws every mesh out of them, so the
// number of binds in a frame does not grow with the number of objects — and the
// day many draws become one indirect call, the data is already in the one layout
// that allows it. A mesh with a buffer of its own could not be drawn that way at
// all.
//
// THE INDICES ARE STORED AS THE CALLER NUMBERED THEM, FROM ZERO. What shifts
// them into the pool is vkCmdDrawIndexed's vertexOffset, which frame.c passes
// from the slot. Rewriting a mesh's indices on the way in would work equally
// well until the same mesh had to be uploaded twice.
//
// INDICES ARE THIRTY-TWO BITS. Sixteen would halve the pool and would refuse the
// first model with more than 65535 vertices in one primitive, which is not a
// thing a loader should have to explain to a person. glTF hands out bytes, shorts
// and ints, and the importer widens all three to this.
//
// NOTHING IS FREED, DELIBERATELY, AND THE HEADER SAYS SO OUT LOUD. There is no
// voe_render_geometry_destroy: the card that unloads a model is the card that
// decides what to do about the hole a removal leaves in a pool, and inventing a
// free list before then would be inventing it blind.
#include "device_internal.h"

#include <base/assert.h>

#include <stdio.h>
#include <stdlib.h>

static bool build_pool(voe_render_device *device, struct voe_render_pool *pool,
		       uint32_t capacity, size_t element,
		       VkBufferUsageFlags usage)
{
	pool->capacity = capacity;
	pool->used = 0;

	return voe_render_buffer_build(device, &pool->buffer,
				       (VkDeviceSize)capacity * element,
				       usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

bool voe_render_geometry_startup(voe_render_device *device)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "starting geometry on no device");
	VOE_BASE_DEBUG_ASSERT(device->capacities.vertices > 0,
			      "a device with room for no vertices");
	VOE_BASE_DEBUG_ASSERT(device->capacities.indices > 0,
			      "a device with room for no indices");
	VOE_BASE_DEBUG_ASSERT(device->capacities.geometries > 0,
			      "a device with room for no meshes");

	// Zeroed, so every slot starts not live with generation 0 — and
	// generation 0 is never handed out, which is what makes a zeroed
	// voe_render_geometry name nothing.
	device->geometries = calloc(device->capacities.geometries,
				    sizeof(*device->geometries));
	VOE_BASE_ASSERT(device->geometries != NULL,
			"out of memory making room for a device's meshes");

	return build_pool(device, &device->vertices,
			  device->capacities.vertices,
			  sizeof(voe_render_vertex),
			  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) &&
	       build_pool(device, &device->indices, device->capacities.indices,
			  sizeof(uint32_t),
			  VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void voe_render_geometry_shutdown(voe_render_device *device)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "shutting geometry down on no device");

	voe_render_buffer_teardown(device, &device->indices.buffer);
	voe_render_buffer_teardown(device, &device->vertices.buffer);
	device->indices.used = 0;
	device->vertices.used = 0;

	free(device->geometries);
	device->geometries = NULL;
}

const struct voe_render_geometry_slot *
voe_render_geometry_at(const voe_render_device *device,
		       voe_render_geometry geometry)
{
	const struct voe_render_geometry_slot *slot;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "asking no device about a mesh");

	if (device->geometries == NULL)
		return NULL;
	if (geometry.index >= device->capacities.geometries)
		return NULL;

	slot = &device->geometries[geometry.index];
	if (!slot->live || slot->generation != geometry.generation)
		return NULL;
	return slot;
}

bool voe_render_geometry_create(voe_render_device *device,
				const voe_render_vertex *vertices,
				uint32_t vertex_count, const uint32_t *indices,
				uint32_t index_count, voe_render_geometry *out,
				voe_base_error *error)
{
	struct voe_render_geometry_slot *slot = NULL;
	uint32_t index = 0;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "uploading a mesh to no device");
	VOE_BASE_DEBUG_ASSERT(vertices != NULL, "uploading a mesh with no vertices");
	VOE_BASE_DEBUG_ASSERT(indices != NULL, "uploading a mesh with no indices");
	VOE_BASE_DEBUG_ASSERT(out != NULL, "uploading a mesh into nothing");
	VOE_BASE_DEBUG_ASSERT(vertex_count > 0 && index_count > 0,
			      "uploading a mesh that has nothing in it");

	for (uint32_t i = 0; i < device->capacities.geometries; i++) {
		if (!device->geometries[i].live) {
			index = i;
			slot = &device->geometries[i];
			break;
		}
	}
	if (slot == NULL) {
		fprintf(stderr,
			"render: all %u mesh slots are taken; the device was made with room for that many\n",
			device->capacities.geometries);
		if (error != NULL)
			*error = VOE_BASE_ERROR_REFUSED;
		return false;
	}

	// Both pools are checked before either is written, so a mesh that does
	// not fit leaves neither pool half filled with it.
	if (device->vertices.used + vertex_count > device->vertices.capacity ||
	    device->indices.used + index_count > device->indices.capacity) {
		fprintf(stderr,
			"render: no room for a mesh of %u vertices and %u indices — %u of %u vertices and %u of %u indices are spent\n",
			vertex_count, index_count, device->vertices.used,
			device->vertices.capacity, device->indices.used,
			device->indices.capacity);
		if (error != NULL)
			*error = VOE_BASE_ERROR_REFUSED;
		return false;
	}

	if (!voe_render_buffer_upload(device, &device->vertices.buffer,
				      (VkDeviceSize)device->vertices.used *
					      sizeof(*vertices),
				      vertices,
				      (VkDeviceSize)vertex_count *
					      sizeof(*vertices)) ||
	    !voe_render_buffer_upload(device, &device->indices.buffer,
				      (VkDeviceSize)device->indices.used *
					      sizeof(*indices),
				      indices,
				      (VkDeviceSize)index_count *
					      sizeof(*indices))) {
		// The pools are left as they were: `used` has not moved, so the
		// bytes a half-finished upload wrote are inside the unspent
		// part of the pool and the next mesh overwrites them.
		if (error != NULL)
			*error = VOE_BASE_ERROR_REFUSED;
		return false;
	}

	slot->first_vertex = device->vertices.used;
	slot->first_index = device->indices.used;
	slot->index_count = index_count;
	slot->generation++;
	slot->live = true;

	device->vertices.used += vertex_count;
	device->indices.used += index_count;

	out->index = index;
	out->generation = slot->generation;
	return true;
}
