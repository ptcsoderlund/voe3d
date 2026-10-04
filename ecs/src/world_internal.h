// The innards of voe_ecs_world, shared by the four files that make one:
// world.c owns the entity slots, component.c the tables, intent.c the queues,
// structure.c the structural queue. Nothing outside ecs/src sees this.
//
// The split is by what a reader is chasing. An id that names the wrong thing is
// world.c's; a row that is not where it should be is component.c's; work that
// never happened is intent.c's; a row that appeared or vanished at the wrong
// moment is structure.c's.
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
// two directions are both a single load. description is whatever the
// registration carried, either marker included, and nothing in this folder
// reads it.
//
// replace is the same kind of thing one step further: the intent a whole row is
// written through and where the row sits in that intent's value. Stored, handed
// back, never read — draining the queue is the owning system's. replace_set is
// what tells a type that was never given one from a type given intent zero.
struct voe_ecs_table {
	const struct voe_ecs_key *key;
	const voe_base_struct_description *description;
	size_t size;
	uint32_t capacity;
	uint32_t count;
	unsigned char *rows;
	voe_ecs_entity *owners;
	uint32_t *row_of;
	voe_ecs_intent replace;
	size_t replace_row_offset;
	bool replace_set;

	// The type's default row, `size` bytes pushed when it is set, NULL until
	// then, and its unsaid row the same way, and its fields' former names,
	// the declaring folder's list, NULL and 0 until set. And the type its rows
	// need beside them, stored and never read here; needs_set tells "none"
	// from type zero. And where a tool offers the type, the declaring folder's
	// string, NULL until set, never read.
	unsigned char *default_row;
	unsigned char *unsaid_row;
	const voe_ecs_former_name *formerly;
	uint32_t formerly_count;
	voe_ecs_type needs;
	bool needs_set;
	const char *menu;
};

// What a structural request asks for.
enum voe_ecs_structure_kind {
	VOE_ECS_STRUCTURE_ADD,
	VOE_ECS_STRUCTURE_REMOVE,
	VOE_ECS_STRUCTURE_DESTROY,
};

// One structural request. An add's row is `table size` bytes at `offset` into
// the world's structure_bytes; a remove and a destroy carry no bytes, and a
// destroy's type is unused.
struct voe_ecs_structure_request {
	enum voe_ecs_structure_kind kind;
	voe_ecs_type type;
	voe_ecs_entity entity;
	uint32_t offset;
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

	// The structural queue. Both arrays are NULL and both capacities zero in
	// a world made without one, which is what makes every submit to it fail.
	// The bytes are appended in submission order and emptied with the
	// requests, so bytes_used is the next add's offset.
	struct voe_ecs_structure_request *structure_requests;
	uint32_t structure_request_capacity;
	uint32_t structure_count;
	unsigned char *structure_bytes;
	uint32_t structure_byte_capacity;
	uint32_t structure_bytes_used;
};

// True when the id names a live slot with a matching generation. The one check
// every function in this folder makes before it touches anything.
bool voe_ecs_world_is_alive(const voe_ecs_world *world, voe_ecs_entity entity);

// Takes the entity's row out of every table it has one in. world.c calls it on a
// destroy; it lives in component.c because the tables are component.c's.
void voe_ecs_component_forget(voe_ecs_world *world, voe_ecs_entity entity);

// How many bytes one value of that intent type is. component.c needs it for a
// replace intent — to check a row fits at the offset it was given, and to say
// how big a buffer a caller has to zero — and it lives here rather than in
// component.c so that the bounds check on an intent id stays in intent.c beside
// every other one.
size_t voe_ecs_intent_value_size(const voe_ecs_world *world,
				 voe_ecs_intent type);
