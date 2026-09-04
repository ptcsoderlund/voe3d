// The component tables: registration, the two directions between an entity and
// its row, and what a removal does to the order.
//
// A ROW IS FOUND BY INDEXING TWICE AND NOT BY SEARCHING. row_of[entity.index] is
// the row, and owners[row] is the entity — one load each way, no hashing and no
// scanning. The cost is one uint32_t per entity slot per registered type, paid
// whether or not anything has that component, and it is paid deliberately: the
// tables in this engine are few and the entity count is bounded, so the memory
// is a number a person can compute and the lookup has no branch in it that
// depends on the data.
//
// A REMOVAL MOVES THE LAST ROW INTO THE HOLE. That is what keeps the rows packed
// so an iteration is a straight walk, and it is why row order is not insertion
// order after anything has been removed. The two arrays and the index are all
// updated together, in one place, because a table with one of the three stale is
// a table that answers confidently with the wrong row.
//
// EVERY ENTRY POINT CHECKS THE GENERATION FIRST. A stale id gets NULL or false,
// never the row of whatever now lives in that slot — which is the whole reason
// an entity id has a generation in it.
#include "world_internal.h"

#include <base/assert.h>

#include <string.h>

static struct voe_ecs_table *table_at(const voe_ecs_world *world,
				      voe_ecs_type type)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "a component table in no world");
	VOE_BASE_ASSERT(type.value < world->table_count,
			"a component type this world never registered");

	// The world is const to a reader, but a table is what a writer writes
	// through, so the cast happens here once rather than at four call sites.
	return &((voe_ecs_world *)world)->tables[type.value];
}

static uint32_t *row_slot(struct voe_ecs_table *table, voe_ecs_entity entity)
{
	return &table->row_of[entity.index];
}

static unsigned char *row_bytes(struct voe_ecs_table *table, uint32_t row)
{
	return table->rows + (size_t)row * table->size;
}

// The whole of a removal, in the one place that knows all three things a
// removal has to keep in step: the packed rows, who owns each one, and where
// each entity's row is. The last row moves into the hole; when the hole was the
// last row, the move is a copy onto itself and costs nothing to allow.
static bool drop(struct voe_ecs_table *table, voe_ecs_entity entity)
{
	uint32_t row = *row_slot(table, entity);
	uint32_t last;

	if (row == VOE_ECS_NO_ROW)
		return false;

	last = --table->count;
	if (row != last) {
		memcpy(row_bytes(table, row), row_bytes(table, last),
		       table->size);
		table->owners[row] = table->owners[last];
		*row_slot(table, table->owners[row]) = row;
	}

	*row_slot(table, entity) = VOE_ECS_NO_ROW;
	return true;
}

voe_ecs_type voe_ecs_component_register(voe_ecs_world *world,
					const struct voe_ecs_key *key,
					size_t size, uint32_t capacity)
{
	struct voe_ecs_table *table;
	voe_ecs_type type;

	VOE_BASE_ASSERT(world != NULL, "registering a component in no world");
	VOE_BASE_ASSERT(key != NULL, "registering a component with no key");
	VOE_BASE_ASSERT(size > 0, "registering a component of no bytes");
	VOE_BASE_ASSERT(capacity > 0, "registering a component with room for none");
	VOE_BASE_ASSERT(world->table_count < world->table_capacity,
			"registering more component types than this world was made for");

	for (uint32_t i = 0; i < world->table_count; i++)
		VOE_BASE_ASSERT(world->tables[i].key != key,
				"registering the same component key twice");

	// Registration is code and not data, so there is nothing here to fail
	// gracefully at: the arena aborts if it cannot allocate (rule 11) and
	// everything else above is a bug in the program's startup.
	type.value = world->table_count++;
	table = &world->tables[type.value];
	table->key = key;
	table->size = size;
	table->capacity = capacity;
	table->count = 0;
	table->rows = voe_base_arena_push(world->arena, (size_t)capacity * size);
	table->owners = voe_base_arena_push(
		world->arena, (size_t)capacity * sizeof(*table->owners));
	table->row_of = voe_base_arena_push(
		world->arena, (size_t)world->entity_capacity * sizeof(*table->row_of));

	for (uint32_t i = 0; i < world->entity_capacity; i++)
		table->row_of[i] = VOE_ECS_NO_ROW;

	return type;
}

voe_ecs_type voe_ecs_component_type(const voe_ecs_world *world,
				    const struct voe_ecs_key *key)
{
	voe_ecs_type type;

	VOE_BASE_DEBUG_ASSERT(world != NULL, "looking up a component in no world");
	VOE_BASE_DEBUG_ASSERT(key != NULL, "looking up a component with no key");

	for (uint32_t i = 0; i < world->table_count; i++) {
		if (world->tables[i].key == key) {
			type.value = i;
			return type;
		}
	}

	VOE_BASE_ASSERT(false,
			"asking for a component type nothing registered — the module's _register was never called");
	type.value = 0;
	return type;
}

bool voe_ecs_component_add(voe_ecs_world *world, voe_ecs_type type,
			   voe_ecs_entity entity, const void *value)
{
	struct voe_ecs_table *table = table_at(world, type);
	uint32_t row;

	VOE_BASE_DEBUG_ASSERT(value != NULL, "adding a component from nothing");

	if (!voe_ecs_world_is_alive(world, entity))
		return false;

	VOE_BASE_ASSERT(*row_slot(table, entity) == VOE_ECS_NO_ROW,
			"adding a component to an entity that already has one of that type");

	if (table->count == table->capacity)
		return false;

	row = table->count++;
	memcpy(row_bytes(table, row), value, table->size);
	table->owners[row] = entity;
	*row_slot(table, entity) = row;
	return true;
}

bool voe_ecs_component_set(voe_ecs_world *world, voe_ecs_type type,
			   voe_ecs_entity entity, const void *value)
{
	struct voe_ecs_table *table = table_at(world, type);
	uint32_t row;

	VOE_BASE_DEBUG_ASSERT(value != NULL, "setting a component from nothing");

	if (!voe_ecs_world_is_alive(world, entity))
		return false;

	row = *row_slot(table, entity);
	if (row == VOE_ECS_NO_ROW)
		return false;

	memcpy(row_bytes(table, row), value, table->size);
	return true;
}

const void *voe_ecs_component_get(const voe_ecs_world *world, voe_ecs_type type,
				  voe_ecs_entity entity)
{
	struct voe_ecs_table *table = table_at(world, type);
	uint32_t row;

	if (!voe_ecs_world_is_alive(world, entity))
		return NULL;

	row = *row_slot(table, entity);
	if (row == VOE_ECS_NO_ROW)
		return NULL;

	return row_bytes(table, row);
}

bool voe_ecs_component_remove(voe_ecs_world *world, voe_ecs_type type,
			      voe_ecs_entity entity)
{
	struct voe_ecs_table *table = table_at(world, type);

	if (!voe_ecs_world_is_alive(world, entity))
		return false;

	return drop(table, entity);
}

uint32_t voe_ecs_component_count(const voe_ecs_world *world, voe_ecs_type type)
{
	return table_at(world, type)->count;
}

const void *voe_ecs_component_rows(const voe_ecs_world *world,
				   voe_ecs_type type)
{
	return table_at(world, type)->rows;
}

const voe_ecs_entity *voe_ecs_component_entities(const voe_ecs_world *world,
						 voe_ecs_type type)
{
	return table_at(world, type)->owners;
}

void voe_ecs_component_forget(voe_ecs_world *world, voe_ecs_entity entity)
{
	// The id is still live at this point — world.c bumps the generation
	// after this returns — so the row lookup below is the entity's own.
	for (uint32_t i = 0; i < world->table_count; i++)
		(void)drop(&world->tables[i], entity);
}
