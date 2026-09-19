// The world and its entity slots: what a create takes, what a destroy gives
// back, and the generation that makes a stale id safe. It also pushes the
// structural queue's two arrays, once, when both structure limits ask for one.
//
// A CREATE PREFERS A SLOT SOMETHING USED TO BE IN. Free slots are taken newest
// first and a slot nothing has ever used is only reached when there are none, so
// generations move as soon as anything is destroyed. Handing out fresh slots
// first would leave every generation at 1 until the world filled up, and the
// bug a generation catches would then only ever appear in a full world.
//
// A DESTROY BUMPS THE GENERATION AND THAT IS WHAT INVALIDATES THE ID. The slot
// goes on the free list, its components are dropped, and the id the caller still
// holds no longer matches — so every function in this folder refuses it.
//
// THE GENERATION WRAPS AND THAT IS ACCEPTED. Four billion creates and destroys
// of one slot bring it back to a number an ancient id would match. Nothing in
// this engine keeps an id that long, and the alternative is a wider id for a
// case nobody has.
#include "world_internal.h"

#include <base/assert.h>

bool voe_ecs_world_is_alive(const voe_ecs_world *world, voe_ecs_entity entity)
{
	if (entity.index >= world->entity_capacity)
		return false;
	if (!world->live[entity.index])
		return false;
	return world->generations[entity.index] == entity.generation;
}

voe_ecs_world *voe_ecs_world_new(voe_base_arena *arena, voe_ecs_limits limits)
{
	voe_ecs_world *world;

	VOE_BASE_ASSERT(arena != NULL, "making a world without an arena");
	VOE_BASE_ASSERT(limits.entities > 0, "a world with room for no entities");
	VOE_BASE_ASSERT(limits.component_types > 0,
			"a world with room for no component types");
	VOE_BASE_ASSERT(limits.intent_types > 0,
			"a world with room for no intent types");

	// One push per array, never a push per element: two pushes are not
	// guaranteed to be adjacent, so an array has to be one of them.
	world = voe_base_arena_push(arena, sizeof(*world));
	world->arena = arena;
	world->entity_capacity = limits.entities;
	world->generations = voe_base_arena_push(
		arena, (size_t)limits.entities * sizeof(*world->generations));
	world->live = voe_base_arena_push(
		arena, (size_t)limits.entities * sizeof(*world->live));
	world->free_slots = voe_base_arena_push(
		arena, (size_t)limits.entities * sizeof(*world->free_slots));

	world->tables = voe_base_arena_push(
		arena, (size_t)limits.component_types * sizeof(*world->tables));
	world->table_capacity = limits.component_types;

	world->queues = voe_base_arena_push(
		arena, (size_t)limits.intent_types * sizeof(*world->queues));
	world->queue_capacity = limits.intent_types;

	// No structural queue unless both limits ask for one: a push of zero
	// bytes asserts, and half a queue (requests with no bytes for an add's
	// row) is not something a caller asked for by name.
	if (limits.structure_requests > 0 && limits.structure_bytes > 0) {
		world->structure_requests = voe_base_arena_push(
			arena, (size_t)limits.structure_requests *
				       sizeof(*world->structure_requests));
		world->structure_request_capacity = limits.structure_requests;
		world->structure_bytes =
			voe_base_arena_push(arena, limits.structure_bytes);
		world->structure_byte_capacity = limits.structure_bytes;
	}

	return world;
}

bool voe_ecs_entity_create(voe_ecs_world *world, voe_ecs_entity *out)
{
	uint32_t slot;

	VOE_BASE_DEBUG_ASSERT(world != NULL, "creating an entity in no world");
	VOE_BASE_DEBUG_ASSERT(out != NULL, "creating an entity into nothing");

	if (world->free_count > 0) {
		slot = world->free_slots[--world->free_count];
	} else if (world->next_slot < world->entity_capacity) {
		slot = world->next_slot++;
	} else {
		return false;
	}

	// The generation is bumped on the claim as well as on the destroy, so
	// that the first id a slot ever hands out has generation 1 and a zeroed
	// voe_ecs_entity matches nothing.
	world->generations[slot]++;
	world->live[slot] = true;
	world->live_count++;

	out->index = slot;
	out->generation = world->generations[slot];
	return true;
}

void voe_ecs_entity_destroy(voe_ecs_world *world, voe_ecs_entity entity)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "destroying an entity in no world");

	// A stale id is not an error here. Two copies of an id in two places is
	// ordinary, and whichever gets here second should not have to know it
	// was second.
	if (!voe_ecs_world_is_alive(world, entity))
		return;

	voe_ecs_component_forget(world, entity);

	world->generations[entity.index]++;
	world->live[entity.index] = false;
	world->live_count--;
	world->free_slots[world->free_count++] = entity.index;
}

bool voe_ecs_entity_alive(const voe_ecs_world *world, voe_ecs_entity entity)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "asking no world about an entity");

	return voe_ecs_world_is_alive(world, entity);
}

uint32_t voe_ecs_entity_count(const voe_ecs_world *world)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "counting the entities in no world");

	return world->live_count;
}
