// The innards of voe_ecs_world, shared by the three files that make one:
// world.c owns the entity slots, component.c the tables, intent.c the queues.
// Nothing outside ecs/src sees this.
//
// The split is by what a reader is chasing. An id that names the wrong thing is
// world.c's; a row that is not where it should be is component.c's; work that
// never happened is intent.c's.
//
// EVERY ARRAY IN HERE IS PUSHED ONCE, AT CREATION, OUT OF THE CALLER'S ARENA.
// Nothing in this folder allocates after that and nothing frees: the world's
// lifetime is the arena's. Two pushes are not guaranteed to be adjacent
// (base/arena.h), which is why every array below is one push of its whole
// length and never a push per element.
#pragma once

#include <ecs/component.h>
#include <ecs/intent.h>
#include <ecs/world.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// What row_of holds for an entity that has no row in that table. It is not a
// valid row because a table's capacity can never reach it.
#define VOE_ECS_NO_ROW UINT32_MAX

// One component type. rows is capacity * size bytes; owners says which entity
// each row belongs to; row_of is one index per entity slot in the world, so the
// two directions are both a single load.
struct voe_ecs_table {
	const struct voe_ecs_key *key;
	size_t size;
	uint32_t capacity;
	uint32_t count;
	unsigned char *rows;
	voe_ecs_entity *owners;
	uint32_t *row_of;
};

// One intent type. values is capacity * size bytes and count is how much of it
// is waiting; there is no read cursor, because a drain is all or nothing.
struct voe_ecs_queue {
	const struct voe_ecs_key *key;
	size_t size;
	uint32_t capacity;
	uint32_t count;
	unsigned char *values;
};

struct voe_ecs_world {
	// The arena the world was made from, kept because a registration
	// happens after creation and has to push the new table's three arrays
	// out of the same arena. Nothing else in here allocates, and this is
	// never used to free anything: the arena's owner does that by rewinding
	// or destroying it.
	voe_base_arena *arena;

	// The entity slots. generations[i] is the generation of slot i and is
	// bumped every time the slot is claimed, so it is odd exactly while the
	// slot is live — but nothing reads it that way, because `live` says so
	// in one place and a reader should not have to know the trick.
	//
	// GENERATION 0 IS NEVER HANDED OUT, so a zeroed voe_ecs_entity names no
	// live entity whatever slot 0 currently holds.
	uint32_t entity_capacity;
	uint32_t *generations;
	bool *live;

	// Slots that were live and are not any more, newest first. A create
	// takes from here before it takes a slot nothing has ever used, which is
	// what makes generations move at all — a world that never reused a slot
	// would never catch a stale id.
	uint32_t *free_slots;
	uint32_t free_count;
	uint32_t next_slot;
	uint32_t live_count;

	struct voe_ecs_table *tables;
	uint32_t table_capacity;
	uint32_t table_count;

	struct voe_ecs_queue *queues;
	uint32_t queue_capacity;
	uint32_t queue_count;
};

// True when the id names a live slot with a matching generation. The one check
// every function in this folder makes before it touches anything.
bool voe_ecs_world_is_alive(const voe_ecs_world *world, voe_ecs_entity entity);

// Takes the entity's row out of every table it has one in. world.c calls it on a
// destroy; it lives in component.c because the tables are component.c's.
void voe_ecs_component_forget(voe_ecs_world *world, voe_ecs_entity entity);
