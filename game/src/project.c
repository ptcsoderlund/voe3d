// A project's types registered and their replace intents applied, as
// game/include/game/project.h gives. The Nth project type takes the Nth of
// game's own intent keys; N is how many types the world holds past the
// engine's VOE_GAME_WORLD_TYPES, because ecs/intent.h can only assert on an
// unregistered key and so cannot be asked which keys are free.
#include <game/project.h>

#include <game/world.h>

#include <base/assert.h>
#include <base/report.h>

#include <ecs/intent.h>

#include <stdalign.h>
#include <string.h>

// The entity at 0 and the row at the next boundary any type can sit on.
#define ROW_OFFSET                                                    \
	((sizeof(voe_ecs_entity) + alignof(max_align_t) - 1) /        \
	 alignof(max_align_t) * alignof(max_align_t))

static const struct voe_ecs_key replace_keys[VOE_GAME_PROJECT_TYPES] = {
	{ "voe_game_project_replace_0" },  { "voe_game_project_replace_1" },
	{ "voe_game_project_replace_2" },  { "voe_game_project_replace_3" },
	{ "voe_game_project_replace_4" },  { "voe_game_project_replace_5" },
	{ "voe_game_project_replace_6" },  { "voe_game_project_replace_7" },
	{ "voe_game_project_replace_8" },  { "voe_game_project_replace_9" },
	{ "voe_game_project_replace_10" }, { "voe_game_project_replace_11" },
	{ "voe_game_project_replace_12" }, { "voe_game_project_replace_13" },
	{ "voe_game_project_replace_14" }, { "voe_game_project_replace_15" },
	{ "voe_game_project_replace_16" }, { "voe_game_project_replace_17" },
	{ "voe_game_project_replace_18" }, { "voe_game_project_replace_19" },
	{ "voe_game_project_replace_20" }, { "voe_game_project_replace_21" },
	{ "voe_game_project_replace_22" }, { "voe_game_project_replace_23" },
	{ "voe_game_project_replace_24" }, { "voe_game_project_replace_25" },
	{ "voe_game_project_replace_26" }, { "voe_game_project_replace_27" },
	{ "voe_game_project_replace_28" }, { "voe_game_project_replace_29" },
	{ "voe_game_project_replace_30" }, { "voe_game_project_replace_31" },
};

// The default a type with no default_row gets.
static const unsigned char zeros[VOE_GAME_PROJECT_ROW];

// How many project types the world holds.
static uint32_t project_types(const voe_ecs_world *world)
{
	uint32_t count = voe_ecs_component_type_count(world);

	VOE_BASE_ASSERT(count >= VOE_GAME_WORLD_TYPES,
			"a project type in a world voe_game_world_new did not make");
	return count - VOE_GAME_WORLD_TYPES;
}

bool voe_game_project_component(voe_ecs_world *world,
				const voe_game_project_type *type)
{
	uint32_t index;
	voe_ecs_type table;
	voe_ecs_intent intent;

	VOE_BASE_ASSERT(world != NULL && type != NULL && type->key != NULL &&
				type->description != NULL,
			"a project type with no world, key or description");

	index = project_types(world);
	if (type->size == 0 || type->size > VOE_GAME_PROJECT_ROW) {
		VOE_BASE_ERROR("game", "%s: a row of %zu bytes, not 1 to %d",
			       type->key->name, type->size,
			       VOE_GAME_PROJECT_ROW);
		return false;
	}
	if (index >= VOE_GAME_PROJECT_TYPES) {
		VOE_BASE_ERROR("game", "%s: more than %d project types",
			       type->key->name, VOE_GAME_PROJECT_TYPES);
		return false;
	}

	table = voe_ecs_component_register(world, type->key, type->size,
					   type->capacity, type->description);
	intent = voe_ecs_intent_register(world, &replace_keys[index],
					 ROW_OFFSET + type->size,
					 VOE_GAME_WORLD_AUTHORED);
	voe_ecs_component_replace_set(world, table, intent, ROW_OFFSET);
	voe_ecs_component_default_set(
		world, table,
		type->default_row != NULL ? type->default_row : zeros);
	if (type->menu != NULL)
		voe_ecs_component_menu_set(world, table, type->menu);
	return true;
}

// Rows go through voe_ecs_component_set, which refuses an entity that is gone
// or has no row: that refusal is the silent drop.
void voe_game_project_replaces_apply(voe_ecs_world *world)
{
	uint32_t types;

	VOE_BASE_ASSERT(world != NULL, "applying project replaces in no world");

	types = project_types(world);
	for (uint32_t i = 0; i < types; i++) {
		voe_ecs_type table = voe_ecs_component_type_at(
			world, VOE_GAME_WORLD_TYPES + i);
		voe_ecs_replace replace = voe_ecs_component_replace(world, table);
		const unsigned char *queue =
			voe_ecs_intent_queue(world, replace.intent);
		uint32_t count = voe_ecs_intent_count(world, replace.intent);

		VOE_BASE_ASSERT(replace.set, "a project type with no replace");
		for (uint32_t j = 0; j < count; j++) {
			const unsigned char *value = queue + j * replace.value_size;
			voe_ecs_entity entity;

			memcpy(&entity, value, sizeof(entity));
			(void)voe_ecs_component_set(world, table, entity,
						    value + replace.row_offset);
		}
		voe_ecs_intent_clear(world, replace.intent);
	}
}
