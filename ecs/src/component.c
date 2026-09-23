// The component tables: registration, the two directions between an entity and
// its row, and what a removal does to the order. Also what a registration is
// given afterwards and only hands back: the replace intent, the default row, the
// type a type needs and the menu path.
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

// Empty, because only their addresses are ever compared — and two distinct
// objects have distinct addresses however alike their contents are.
const voe_base_struct_description voe_ecs_runtime_only = { 0 };
const voe_base_struct_description voe_ecs_description_compiled_out = { 0 };

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

voe_ecs_type voe_ecs_component_register(
	voe_ecs_world *world, const struct voe_ecs_key *key, size_t size,
	uint32_t capacity, const voe_base_struct_description *description)
{
	struct voe_ecs_table *table;
	voe_ecs_type type;

	VOE_BASE_ASSERT(world != NULL, "registering a component in no world");
	VOE_BASE_ASSERT(key != NULL, "registering a component with no key");
	// The header declares this parameter nonnull, so from -O1 up clang takes
	// the comparison as already true and deletes it. It stands in Debug, which
	// is what check.cmake and daily work build; in every build a literal NULL
	// is a compile error, which is the half that catches the forgotten one.
	VOE_BASE_ASSERT(description != NULL,
			"registering a component with no description — pass its description, &voe_ecs_runtime_only or &voe_ecs_description_compiled_out (ecs/component.h)");
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
	table->description = description;
	table->size = size;
	table->capacity = capacity;
	table->count = 0;
	table->rows = voe_base_arena_push(world->arena, (size_t)capacity * size);
	table->owners = voe_base_arena_push(
		world->arena, (size_t)capacity * sizeof(*table->owners));
	table->row_of = voe_base_arena_push(
		world->arena, (size_t)world->entity_capacity * sizeof(*table->row_of));
	// No replace intent until the folder names one, which it cannot do here:
	// the intent it would name has not been registered yet.
	table->replace = (voe_ecs_intent){ 0 };
	table->replace_row_offset = 0;
	table->replace_set = false;
	table->default_row = NULL;
	table->needs = (voe_ecs_type){ 0 };
	table->needs_set = false;
	table->menu = NULL;

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

uint32_t voe_ecs_component_type_count(const voe_ecs_world *world)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "counting component types in no world");

	return world->table_count;
}

voe_ecs_type voe_ecs_component_type_at(const voe_ecs_world *world,
				       uint32_t index)
{
	voe_ecs_type type;

	VOE_BASE_DEBUG_ASSERT(world != NULL, "listing component types in no world");
	VOE_BASE_ASSERT(index < world->table_count,
			"a component type index at or past the number registered");

	// A type is its table's index, so the index-th registration is the type.
	type.value = index;
	return type;
}

const struct voe_ecs_key *voe_ecs_component_key(const voe_ecs_world *world,
						voe_ecs_type type)
{
	return table_at(world, type)->key;
}

const voe_base_struct_description *
voe_ecs_component_description(const voe_ecs_world *world, voe_ecs_type type)
{
	const voe_base_struct_description *description =
		table_at(world, type)->description;

	// NULL for both markers, so a caller that expands fields — the inspector
	// — needs to know about neither of them.
	if (description == &voe_ecs_runtime_only ||
	    description == &voe_ecs_description_compiled_out)
		return NULL;

	return description;
}

bool voe_ecs_component_runtime_only(const voe_ecs_world *world,
				    voe_ecs_type type)
{
	return table_at(world, type)->description == &voe_ecs_runtime_only;
}

// THE THREE CHECKS ARE COMPARISONS OF SIZES THIS FOLDER ALREADY HOLDS, and that
// is the whole of what can be checked here: the value is bytes to ecs, so an
// offset that lands on the wrong field of the right size is a mistake only the
// declaring folder can see. What these catch is the mistake anyone can make once
// — offsetof of the entity instead of the row, or a row pointed past the end of
// an intent that turned out to be a different one.
void voe_ecs_component_replace_set(voe_ecs_world *world, voe_ecs_type type,
				   voe_ecs_intent intent, size_t row_offset)
{
	struct voe_ecs_table *table = table_at(world, type);

	VOE_BASE_ASSERT(!table->replace_set,
			"giving a component type a second replace intent");
	VOE_BASE_ASSERT(row_offset >= sizeof(voe_ecs_entity),
			"a replace intent's row would sit over the entity at offset zero");
	VOE_BASE_ASSERT(row_offset + table->size <=
				voe_ecs_intent_value_size(world, intent),
			"a replace intent's value has no room for a whole row at that offset");

	table->replace = intent;
	table->replace_row_offset = row_offset;
	table->replace_set = true;
}

voe_ecs_replace voe_ecs_component_replace(const voe_ecs_world *world,
					  voe_ecs_type type)
{
	const struct voe_ecs_table *table = table_at(world, type);
	voe_ecs_replace replace = { 0 };

	// A type nobody named an intent for is shown and not edited, so every
	// number a caller would size a buffer with stays zero.
	if (!table->replace_set)
		return replace;

	replace.set = true;
	replace.intent = table->replace;
	replace.row_offset = table->replace_row_offset;
	replace.row_size = table->size;
	// Read now rather than kept beside the offset: it is the queue's own
	// number and copying it here would be a second place for it to be wrong.
	replace.value_size = voe_ecs_intent_value_size(world, table->replace);
	return replace;
}

void voe_ecs_component_default_set(voe_ecs_world *world, voe_ecs_type type,
				   const void *row)
{
	struct voe_ecs_table *table = table_at(world, type);

	VOE_BASE_ASSERT(row != NULL, "setting a component's default from nothing");
	VOE_BASE_ASSERT(table->default_row == NULL,
			"giving a component type a second default row");

	// Copied, so the caller's row can be a local: the world's copy lives as
	// long as the world does, in the world's arena.
	table->default_row = voe_base_arena_push(world->arena, table->size);
	memcpy(table->default_row, row, table->size);
}

const void *voe_ecs_component_default(const voe_ecs_world *world,
				      voe_ecs_type type)
{
	return table_at(world, type)->default_row;
}

void voe_ecs_component_needs_set(voe_ecs_world *world, voe_ecs_type type,
				 voe_ecs_type needed)
{
	struct voe_ecs_table *table = table_at(world, type);

	(void)table_at(world, needed);
	VOE_BASE_ASSERT(!table->needs_set,
			"giving a component type a second type it needs");

	table->needs = needed;
	table->needs_set = true;
}

bool voe_ecs_component_needs(const voe_ecs_world *world, voe_ecs_type type,
			     voe_ecs_type *out)
{
	const struct voe_ecs_table *table = table_at(world, type);

	VOE_BASE_DEBUG_ASSERT(out != NULL, "asking what a type needs into nothing");

	if (!table->needs_set)
		return false;

	*out = table->needs;
	return true;
}

void voe_ecs_component_menu_set(voe_ecs_world *world, voe_ecs_type type,
				const char *path)
{
	struct voe_ecs_table *table = table_at(world, type);

	VOE_BASE_ASSERT(path != NULL && path[0] != '\0',
			"giving a component type an empty menu path");
	VOE_BASE_ASSERT(table->menu == NULL,
			"giving a component type a second menu path");

	table->menu = path;
}

const char *voe_ecs_component_menu(const voe_ecs_world *world,
				   voe_ecs_type type)
{
	return table_at(world, type)->menu;
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
