// The structural queue: submitting appends a request (and an add's row bytes),
// applying walks them in order against the tables and entity slots, then empties
// both arrays.
//
// APPLYING GOES THROUGH THE SAME CALLS ANYONE ELSE WOULD MAKE. An add is
// voe_ecs_component_add, a remove voe_ecs_component_remove, a destroy
// voe_ecs_entity_destroy; the only thing done first is the one check _add would
// assert on — a row of that type already there — because here that is an
// ordinary drop and not a caller's bug.
//
// A FULL QUEUE REFUSES WHOLE REQUESTS. An add that fits in the request array but
// not in the bytes is refused outright, so nothing half-queued is ever applied.
#include "world_internal.h"

#include <ecs/structure.h>

#include <base/assert.h>

#include <string.h>

// Appends one request carrying `size` bytes from `row` (none when size is 0).
static bool submit(voe_ecs_world *world, struct voe_ecs_structure_request request,
		   const void *row, size_t size)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "a structural request to no world");

	if (world->structure_count == world->structure_request_capacity)
		return false;
	if (size > world->structure_byte_capacity - world->structure_bytes_used)
		return false;

	request.offset = world->structure_bytes_used;
	if (size > 0) {
		memcpy(world->structure_bytes + request.offset, row, size);
		world->structure_bytes_used += (uint32_t)size;
	}
	world->structure_requests[world->structure_count++] = request;
	return true;
}

bool voe_ecs_structure_add(voe_ecs_world *world, voe_ecs_type type,
			   voe_ecs_entity entity, const void *row)
{
	VOE_BASE_ASSERT(row != NULL, "queuing an add of no row");
	VOE_BASE_ASSERT(type.value < world->table_count,
			"queuing an add of a type this world never registered");

	return submit(world,
		      (struct voe_ecs_structure_request){
			      .kind = VOE_ECS_STRUCTURE_ADD,
			      .type = type,
			      .entity = entity },
		      row, world->tables[type.value].size);
}

bool voe_ecs_structure_remove(voe_ecs_world *world, voe_ecs_type type,
			      voe_ecs_entity entity)
{
	VOE_BASE_ASSERT(type.value < world->table_count,
			"queuing a remove of a type this world never registered");

	return submit(world,
		      (struct voe_ecs_structure_request){
			      .kind = VOE_ECS_STRUCTURE_REMOVE,
			      .type = type,
			      .entity = entity },
		      NULL, 0);
}

bool voe_ecs_structure_destroy(voe_ecs_world *world, voe_ecs_entity entity)
{
	return submit(world,
		      (struct voe_ecs_structure_request){
			      .kind = VOE_ECS_STRUCTURE_DESTROY,
			      .entity = entity },
		      NULL, 0);
}

void voe_ecs_structure_apply(voe_ecs_world *world)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "applying the structure of no world");

	for (uint32_t i = 0; i < world->structure_count; i++) {
		const struct voe_ecs_structure_request *request =
			&world->structure_requests[i];

		switch (request->kind) {
		case VOE_ECS_STRUCTURE_ADD:
			// Dead entity and full table are _add's false; an
			// existing row is the one case it would assert on.
			if (voe_ecs_component_get(world, request->type,
						  request->entity) != NULL)
				break;
			(void)voe_ecs_component_add(
				world, request->type, request->entity,
				world->structure_bytes + request->offset);
			break;
		case VOE_ECS_STRUCTURE_REMOVE:
			(void)voe_ecs_component_remove(world, request->type,
						       request->entity);
			break;
		case VOE_ECS_STRUCTURE_DESTROY:
			voe_ecs_entity_destroy(world, request->entity);
			break;
		}
	}

	world->structure_count = 0;
	world->structure_bytes_used = 0;
}

uint32_t voe_ecs_structure_count(const voe_ecs_world *world)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "counting the structure of no world");

	return world->structure_count;
}
