// The geometry pools and the ranges into them. Two static pools — one vertex,
// one index, both device-local, ranges taken and given back — plus a
// transient pair in every frame slot, host-visible and emptied every frame; and
// one slot per range so that a voe_render_geometry id can be refused when it
// names nothing.
//
// A MESH IS A RANGE AND NOT A BUFFER, AND THAT IS THE WHOLE POINT OF THIS FILE.
// A frame binds a pool pair once and then draws every mesh in it out of them, so
// the number of binds in a frame does not grow with the number of objects — and
// the day many draws become one indirect call, the data is already in the one
// layout that allows it. A mesh with a buffer of its own could not be drawn that
// way at all.
//
// THERE ARE TWO LIFETIMES OF GEOMETRY AND THE ID DOES NOT TELL THEM APART. A
// static range is voe_render_geometry_create's: uploaded once, at startup, and
// kept for the life of the device. A transient range is
// voe_render_geometry_create_transient's: written straight into the open frame's
// slot, drawn in that frame, and gone when the next frame begins. Both are a
// voe_render_geometry_slot and both go through draw_with in draw.c, which is
// what keeps the draw path single; the slot's `transient` flag is the only place
// the difference is recorded, and the draw reads it to know which pair of
// buffers to have bound.
//
// THE SLOT TABLE IS TWO BANDS. [0, capacities.geometries) is the static band and
// [capacities.geometries, + capacities.transient_geometries) is the transient
// one. Each create walks its own band; voe_render_geometry_at checks against the
// total and otherwise treats every slot alike, which is why that function did
// not change when the second kind arrived.
//
// THE INDICES ARE STORED AS THE CALLER NUMBERED THEM, FROM ZERO. What shifts
// them into a pool is vkCmdDrawIndexed's vertexOffset, which draw.c passes from
// the slot. Rewriting a mesh's indices on the way in would work equally well
// until the same mesh had to be uploaded twice. The same holds in the transient
// pools, where it is what lets a caller build the same arrays for either create.
//
// INDICES ARE THIRTY-TWO BITS. Sixteen would halve the pool and would refuse the
// first model with more than 65535 vertices in one primitive, which is not a
// thing a loader should have to explain to a person. glTF hands out bytes, shorts
// and ints, and the importer widens all three to this.
//
// A STATIC RANGE IS FREED BY voe_render_geometry_destroy (0278), so a reloaded
// model does not fill the pools. Each static pool keeps a sorted list of the
// ranges given back; a create takes the first that fits, splitting it, before
// appending at `used`, and a range given back merges with a neighbour it
// touches or lowers `used`. First fit and no compaction: fragmentation that
// leaves no hole big enough is a returned failure, lifted only by a compacting
// pass that rewrites every slot. The transient path frees nothing because it
// empties everything.
//
// BOTH CREATES KEEP THE RANGE'S BOUNDING SPHERE, from point_shadow_faces.h, for
// the point-shadow draw's face culling (ADR-0325 point 2).
#include "device_internal.h"
#include "point_shadow_faces.h"

#include <base/assert.h>
#include <base/report.h>

#include <stdlib.h>
#include <string.h>

// A static pool, with room for as many holes as the static band has slots —
// the bound device_parts.h gives for voe_render_pool.
static bool build_pool(voe_render_device *device, struct voe_render_pool *pool,
		       uint32_t capacity, size_t element,
		       VkBufferUsageFlags usage)
{
	pool->capacity = capacity;
	pool->used = 0;
	pool->hole_count = 0;
	pool->hole_room = device->capacities.geometries;
	pool->holes = calloc(pool->hole_room, sizeof(*pool->holes));
	VOE_BASE_ASSERT(pool->holes != NULL,
			"out of memory making room for a pool's free ranges");

	return voe_render_buffer_build(device, &pool->buffer,
				       (VkDeviceSize)capacity * element,
				       usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

// One transient pool: host-visible and coherent, mapped once here and left
// mapped, the same way descriptors.c makes the per-slot buffers and for the same
// reason — it is written every frame, and mapping round each write would be two
// driver calls to say what one kept pointer says.
static bool build_transient_pool(voe_render_device *device,
				 struct voe_render_transient_pool *pool,
				 uint32_t capacity, size_t element,
				 VkBufferUsageFlags usage)
{
	VkResult result;

	pool->pool.capacity = capacity;
	pool->pool.used = 0;
	pool->mapped = NULL;

	if (!voe_render_buffer_build(device, &pool->pool.buffer,
				     (VkDeviceSize)capacity * element, usage,
				     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
		return false;

	// vkMapMemory hands its pointer back through a void **, which is
	// Vulkan's signature and not one this engine gets to choose.
	result = voe_render_vk.map_memory(device->device,
					  pool->pool.buffer.memory, 0,
					  VK_WHOLE_SIZE, 0, &pool->mapped);
	if (result != VK_SUCCESS || pool->mapped == NULL) {
		VOE_BASE_ERROR("render",
			       "vkMapMemory failed on a transient geometry pool (VkResult %d)",
			       (int)result);
		pool->mapped = NULL;
		return false;
	}
	return true;
}

// Safe on a pool that was never built, which is every slot's on a device that
// asked for no transient room.
static void teardown_transient_pool(voe_render_device *device,
				    struct voe_render_transient_pool *pool)
{
	if (pool->mapped != NULL) {
		voe_render_vk.unmap_memory(device->device,
					   pool->pool.buffer.memory);
		pool->mapped = NULL;
	}
	voe_render_buffer_teardown(device, &pool->pool.buffer);
	pool->pool.used = 0;
	pool->pool.capacity = 0;
}

bool voe_render_geometry_startup(voe_render_device *device)
{
	uint32_t total;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "starting geometry on no device");
	VOE_BASE_DEBUG_ASSERT(device->capacities.vertices > 0,
			      "a device with room for no vertices");
	VOE_BASE_DEBUG_ASSERT(device->capacities.indices > 0,
			      "a device with room for no indices");
	VOE_BASE_DEBUG_ASSERT(device->capacities.geometries > 0,
			      "a device with room for no meshes");
	// The transient trio is all nought or all something. Room for ranges
	// with no bytes to put in them, or bytes with no range to name them, is
	// a capacities struct half filled in, and that is the caller's bug
	// rather than a device to open.
	VOE_BASE_ASSERT((device->capacities.transient_geometries == 0) ==
					(device->capacities.transient_vertices == 0) &&
				(device->capacities.transient_geometries == 0) ==
					(device->capacities.transient_indices == 0),
			"the three transient capacities are asked for together or not at all");

	// Zeroed, so every slot starts not live with generation 0 — and
	// generation 0 is never handed out, which is what makes a zeroed
	// voe_render_geometry name nothing.
	total = device->capacities.geometries +
		device->capacities.transient_geometries;
	device->geometries = calloc(total, sizeof(*device->geometries));
	VOE_BASE_ASSERT(device->geometries != NULL,
			"out of memory making room for a device's meshes");

	// The transient band starts at generation 1 and not 0, because a
	// transient create hands out the generation the slot already has: it is
	// the reset at the top of the next frame that moves it on, not the
	// create. Starting at 1 keeps generation 0 unissued for these slots too.
	for (uint32_t i = device->capacities.geometries; i < total; i++) {
		device->geometries[i].generation = 1;
		device->geometries[i].transient = true;
	}

	if (!build_pool(device, &device->vertices, device->capacities.vertices,
			sizeof(voe_render_vertex),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) ||
	    !build_pool(device, &device->indices, device->capacities.indices,
			sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT))
		return false;

	if (device->capacities.transient_geometries == 0)
		return true;

	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		struct voe_render_frame *frame = &device->frames[i];

		if (!build_transient_pool(device, &frame->transient_vertices,
					  device->capacities.transient_vertices,
					  sizeof(voe_render_vertex),
					  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) ||
		    !build_transient_pool(device, &frame->transient_indices,
					  device->capacities.transient_indices,
					  sizeof(uint32_t),
					  VK_BUFFER_USAGE_INDEX_BUFFER_BIT))
			return false;
	}
	return true;
}

void voe_render_geometry_shutdown(voe_render_device *device)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "shutting geometry down on no device");

	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		teardown_transient_pool(device,
					&device->frames[i].transient_indices);
		teardown_transient_pool(device,
					&device->frames[i].transient_vertices);
	}

	voe_render_buffer_teardown(device, &device->indices.buffer);
	voe_render_buffer_teardown(device, &device->vertices.buffer);
	device->indices.used = 0;
	device->vertices.used = 0;
	free(device->indices.holes);
	free(device->vertices.holes);
	device->indices.holes = NULL;
	device->vertices.holes = NULL;
	device->indices.hole_count = 0;
	device->vertices.hole_count = 0;

	free(device->geometries);
	device->geometries = NULL;
}

// THE TRANSIENT RESET IS THE STALENESS CHECK FIRING ON A SCHEDULE. At the top of
// every frame, voe_render_geometry_frame_reset makes every live transient slot
// not live and moves its generation on, so every transient id handed out last
// frame names nothing from then on — refused by the same generation comparison
// that refuses any stale id, with no second mechanism. Only the live slots move,
// which keeps the generation counter to one step per range per frame.
//
// THE GENERATION CAN WRAP AND NOTHING HERE HANDLES IT. A uint32_t moves on once
// per frame per transient range in use; at sixty frames a second that is over
// two years before a slot comes round to a generation it has handed out before,
// at which point an id kept for two years would name a live range. Noted, not
// handled: the fix is a wider counter and it is not worth a branch in every
// create today.
void voe_render_geometry_frame_reset(voe_render_device *device,
				     struct voe_render_frame *frame)
{
	uint32_t first;
	uint32_t end;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "resetting geometry on no device");
	VOE_BASE_DEBUG_ASSERT(frame != NULL, "resetting geometry for no frame slot");

	first = device->capacities.geometries;
	end = first + device->capacities.transient_geometries;

	// Only the live ones, so the counter moves by how many ranges a frame
	// built and not by the size of the band. The wrap this can eventually
	// reach is in the header; it is noted there and not handled here.
	for (uint32_t i = first; i < end; i++) {
		struct voe_render_geometry_slot *slot = &device->geometries[i];

		if (!slot->live)
			continue;
		slot->live = false;
		slot->generation++;
	}

	// Emptied and not cleared: the bytes are overwritten by whatever this
	// frame builds, and the fence the caller has just waited on is what says
	// the card has stopped reading them.
	frame->transient_vertices.pool.used = 0;
	frame->transient_indices.pool.used = 0;
}

const struct voe_render_geometry_slot *
voe_render_geometry_at(const voe_render_device *device,
		       voe_render_geometry geometry)
{
	const struct voe_render_geometry_slot *slot;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "asking no device about a mesh");

	if (device->geometries == NULL)
		return NULL;
	if (geometry.index >= device->capacities.geometries +
				      device->capacities.transient_geometries)
		return NULL;

	slot = &device->geometries[geometry.index];
	if (!slot->live || slot->generation != geometry.generation)
		return NULL;
	return slot;
}

// Where `count` elements would go in a static pool, first fit: the index of the
// lowest hole that holds them, hole_count for the end of what is spent, or
// UINT32_MAX when neither has room. Finding is apart from taking so that both
// pools can be checked before either is changed.
static uint32_t pool_find(const struct voe_render_pool *pool, uint32_t count)
{
	for (uint32_t i = 0; i < pool->hole_count; i++) {
		if (pool->holes[i].count >= count)
			return i;
	}
	if (count <= pool->capacity - pool->used)
		return pool->hole_count;
	return UINT32_MAX;
}

static uint32_t pool_offset(const struct voe_render_pool *pool, uint32_t where)
{
	return where < pool->hole_count ? pool->holes[where].offset : pool->used;
}

// Takes what pool_find found. A hole is split from its low end; what is left of
// it still touches nothing, so the list's invariants hold without a merge.
static void pool_take(struct voe_render_pool *pool, uint32_t where,
		      uint32_t count)
{
	struct voe_render_free_range *hole;

	VOE_BASE_DEBUG_ASSERT(where <= pool->hole_count, "taking a range pool_find did not find");

	if (where == pool->hole_count) {
		pool->used += count;
		return;
	}
	hole = &pool->holes[where];
	VOE_BASE_DEBUG_ASSERT(hole->count >= count, "taking more than a hole holds");
	hole->offset += count;
	hole->count -= count;
	if (hole->count > 0)
		return;
	memmove(hole, hole + 1,
		(size_t)(pool->hole_count - where - 1) * sizeof(*hole));
	pool->hole_count--;
}

// Gives a range back: it lowers `used` when it ends there (taking the hole
// below with it if that now ends there too), else it joins the sorted list,
// merged with whichever neighbours it touches.
static void pool_give(struct voe_render_pool *pool, uint32_t offset,
		      uint32_t count)
{
	struct voe_render_free_range *holes = pool->holes;
	uint32_t at = 0;
	bool below;
	bool above;

	VOE_BASE_DEBUG_ASSERT(count > 0 && offset + count <= pool->used,
			      "giving back a range the pool never handed out");

	while (at < pool->hole_count && holes[at].offset < offset)
		at++;

	if (offset + count == pool->used) {
		VOE_BASE_DEBUG_ASSERT(at == pool->hole_count, "a hole above the end of the pool");
		pool->used = offset;
		if (at > 0 && holes[at - 1].offset + holes[at - 1].count == offset) {
			pool->used = holes[at - 1].offset;
			pool->hole_count--;
		}
		return;
	}

	below = at > 0 && holes[at - 1].offset + holes[at - 1].count == offset;
	above = at < pool->hole_count && offset + count == holes[at].offset;
	if (below && above) {
		holes[at - 1].count += count + holes[at].count;
		memmove(&holes[at], &holes[at + 1],
			(size_t)(pool->hole_count - at - 1) * sizeof(*holes));
		pool->hole_count--;
	} else if (below) {
		holes[at - 1].count += count;
	} else if (above) {
		holes[at].offset = offset;
		holes[at].count += count;
	} else {
		VOE_BASE_ASSERT(pool->hole_count < pool->hole_room,
				"more free ranges than live meshes, which the list's invariants rule out");
		memmove(&holes[at + 1], &holes[at],
			(size_t)(pool->hole_count - at) * sizeof(*holes));
		holes[at] = (struct voe_render_free_range){ offset, count };
		pool->hole_count++;
	}
}

bool voe_render_geometry_create(voe_render_device *device,
				const voe_render_vertex *vertices,
				uint32_t vertex_count, const uint32_t *indices,
				uint32_t index_count, voe_render_geometry *out,
				voe_base_error *error)
{
	struct voe_render_geometry_slot *slot = NULL;
	uint32_t index = 0;
	uint32_t vertex_at;
	uint32_t index_at;
	uint32_t first_vertex;
	uint32_t first_index;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "uploading a mesh to no device");
	VOE_BASE_DEBUG_ASSERT(vertices != NULL, "uploading a mesh with no vertices");
	VOE_BASE_DEBUG_ASSERT(indices != NULL, "uploading a mesh with no indices");
	VOE_BASE_DEBUG_ASSERT(out != NULL, "uploading a mesh into nothing");
	VOE_BASE_DEBUG_ASSERT(vertex_count > 0 && index_count > 0,
			      "uploading a mesh that has nothing in it");

	// The static band only. The transient one above it is another create's.
	for (uint32_t i = 0; i < device->capacities.geometries; i++) {
		if (!device->geometries[i].live) {
			index = i;
			slot = &device->geometries[i];
			break;
		}
	}
	if (slot == NULL) {
		VOE_BASE_ERROR("render",
			       "all %u mesh slots are taken; the device was made with room for that many",
			       device->capacities.geometries);
		if (error != NULL)
			*error = VOE_BASE_ERROR_REFUSED;
		return false;
	}

	// Both pools are checked before either is written, so a mesh that does
	// not fit leaves neither pool half filled with it.
	vertex_at = pool_find(&device->vertices, vertex_count);
	index_at = pool_find(&device->indices, index_count);
	if (vertex_at == UINT32_MAX || index_at == UINT32_MAX) {
		VOE_BASE_ERROR("render",
			       "no room for a mesh of %u vertices and %u indices — no free range fits and the pools are spent to %u of %u vertices and %u of %u indices",
			       vertex_count, index_count, device->vertices.used,
			       device->vertices.capacity, device->indices.used,
			       device->indices.capacity);
		if (error != NULL)
			*error = VOE_BASE_ERROR_REFUSED;
		return false;
	}
	first_vertex = pool_offset(&device->vertices, vertex_at);
	first_index = pool_offset(&device->indices, index_at);

	if (!voe_render_buffer_upload(device, &device->vertices.buffer,
				      (VkDeviceSize)first_vertex *
					      sizeof(*vertices),
				      vertices,
				      (VkDeviceSize)vertex_count *
					      sizeof(*vertices)) ||
	    !voe_render_buffer_upload(device, &device->indices.buffer,
				      (VkDeviceSize)first_index *
					      sizeof(*indices),
				      indices,
				      (VkDeviceSize)index_count *
					      sizeof(*indices))) {
		// The pools are left as they were: nothing has been taken, so
		// the bytes a half-finished upload wrote are in a free part of
		// the pool and the next mesh there overwrites them.
		if (error != NULL)
			*error = VOE_BASE_ERROR_REFUSED;
		return false;
	}

	pool_take(&device->vertices, vertex_at, vertex_count);
	pool_take(&device->indices, index_at, index_count);

	slot->sphere = voe_render_point_shadow_sphere(vertices, vertex_count);
	slot->first_vertex = first_vertex;
	slot->first_index = first_index;
	slot->index_count = index_count;
	slot->vertex_count = vertex_count;
	slot->generation++;
	slot->live = true;
	slot->transient = false;

	out->index = index;
	out->generation = slot->generation;
	return true;
}

// Waits for idle before giving the ranges back, because the next create may
// upload over them while a frame in flight is still drawing from them.
bool voe_render_geometry_destroy(voe_render_device *device,
				 voe_render_geometry geometry)
{
	struct voe_render_geometry_slot *slot;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "destroying a mesh on no device");
	VOE_BASE_ASSERT(geometry.index < device->capacities.geometries ||
				geometry.index >= device->capacities.geometries +
							  device->capacities.transient_geometries,
			"destroying a transient mesh — it is gone at the next frame's begin, and nothing destroys one");

	if (device->geometries == NULL ||
	    geometry.index >= device->capacities.geometries)
		return false;
	slot = &device->geometries[geometry.index];
	if (!slot->live || slot->generation != geometry.generation)
		return false;

	voe_render_vk.device_wait_idle(device->device);

	pool_give(&device->vertices, slot->first_vertex, slot->vertex_count);
	pool_give(&device->indices, slot->first_index, slot->index_count);

	// Bumped here as well as on a create, as a texture's is, so the id just
	// destroyed is refused at once and not only once the slot is reused.
	slot->generation++;
	slot->live = false;
	return true;
}

// THE TRANSIENT POOLS NEED NO BARRIER AND NO WAIT, AND ONE APPEARING HERE WOULD
// BE THE BUG. They are per frame slot, so what a create writes into is memory
// the card finished reading two frames ago — which the fence at the top of
// voe_render_frame_begin has already waited for before the reset empties them.
// That is the same fact that makes the object buffer in the same slot safe to
// overwrite, and it is the only synchronisation story in geometry.c.
bool voe_render_geometry_create_transient(voe_render_device *device,
					  const voe_render_vertex *vertices,
					  uint32_t vertex_count,
					  const uint32_t *indices,
					  uint32_t index_count,
					  voe_render_geometry *out,
					  voe_base_error *error)
{
	struct voe_render_frame *frame;
	struct voe_render_transient_pool *vertex_pool;
	struct voe_render_transient_pool *index_pool;
	struct voe_render_geometry_slot *slot = NULL;
	uint32_t first;
	uint32_t end;
	uint32_t index = 0;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "building transient geometry on no device");
	VOE_BASE_DEBUG_ASSERT(vertices != NULL, "building transient geometry with no vertices");
	VOE_BASE_DEBUG_ASSERT(indices != NULL, "building transient geometry with no indices");
	VOE_BASE_DEBUG_ASSERT(out != NULL, "building transient geometry into nothing");
	VOE_BASE_DEBUG_ASSERT(vertex_count > 0 && index_count > 0,
			      "building transient geometry that has nothing in it");
	// The slot this writes into is the one _begin picked, and its memory is
	// only known to be free of the card once _begin has waited for it — so
	// there is no frame to build into until a frame is open.
	VOE_BASE_ASSERT(device->recording,
			"building transient geometry with no frame open — it lives in the slot voe_render_frame_begin picks, so it is built between _begin and _end and nowhere else");

	if (device->capacities.transient_geometries == 0) {
		VOE_BASE_ERROR("render",
			       "this device was opened with no transient geometry room — the transient_ capacities are nought");
		if (error != NULL)
			*error = VOE_BASE_ERROR_REFUSED;
		return false;
	}

	// The transient band only.
	first = device->capacities.geometries;
	end = first + device->capacities.transient_geometries;
	for (uint32_t i = first; i < end; i++) {
		if (!device->geometries[i].live) {
			index = i;
			slot = &device->geometries[i];
			break;
		}
	}
	if (slot == NULL) {
		VOE_BASE_ERROR("render",
			       "all %u transient mesh slots are taken this frame; transient_geometries is too small for what this frame builds",
			       device->capacities.transient_geometries);
		if (error != NULL)
			*error = VOE_BASE_ERROR_REFUSED;
		return false;
	}

	frame = voe_render_frame_open(device);
	vertex_pool = &frame->transient_vertices;
	index_pool = &frame->transient_indices;

	// Both pools are checked before either is written, as the static create
	// does, so a refusal leaves this frame's pools exactly as they were and
	// every range already built in them still draws.
	if (vertex_pool->pool.used + vertex_count > vertex_pool->pool.capacity ||
	    index_pool->pool.used + index_count > index_pool->pool.capacity) {
		VOE_BASE_ERROR("render",
			       "no room this frame for transient geometry of %u vertices and %u indices — %u of %u transient vertices and %u of %u transient indices are spent; the transient_ capacities are too small for what this frame builds",
			       vertex_count, index_count, vertex_pool->pool.used,
			       vertex_pool->pool.capacity, index_pool->pool.used,
			       index_pool->pool.capacity);
		if (error != NULL)
			*error = VOE_BASE_ERROR_REFUSED;
		return false;
	}

	// Straight into the mapped pool at the end of what this frame has built
	// so far. Coherent memory, so there is nothing to flush; write-combined
	// memory, so nothing below reads it back.
	memcpy((unsigned char *)vertex_pool->mapped +
		       (size_t)vertex_pool->pool.used * sizeof(*vertices),
	       vertices, (size_t)vertex_count * sizeof(*vertices));
	memcpy((unsigned char *)index_pool->mapped +
		       (size_t)index_pool->pool.used * sizeof(*indices),
	       indices, (size_t)index_count * sizeof(*indices));

	// The generation is the one the slot already carries: the reset at the
	// top of the next frame is what moves it on, and it is what makes this
	// id stale then. See the header.
	slot->sphere = voe_render_point_shadow_sphere(vertices, vertex_count);
	slot->first_vertex = vertex_pool->pool.used;
	slot->first_index = index_pool->pool.used;
	slot->index_count = index_count;
	slot->live = true;
	slot->transient = true;

	vertex_pool->pool.used += vertex_count;
	index_pool->pool.used += index_count;

	out->index = index;
	out->generation = slot->generation;
	return true;
}
