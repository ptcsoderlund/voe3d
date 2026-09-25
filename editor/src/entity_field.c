// The choices an ENTITY field's dropdown lists and the name each is shown by,
// read out of the identity table. See the header for why names and why only
// authored entities.
#include "entity_field.h"

#include "inspector_value.h"

#include <base/assert.h>

#include <game/world.h>

#include <scene/identity_component.h>

#include <stdint.h>

// The word a field pointing at nothing says. A literal, because a label's text
// is read after the call that made it has returned.
#define NONE_TEXT "None"

uint32_t voe_editor_entity_field_choices(const voe_ecs_world *world,
					 voe_ecs_entity *out, uint32_t room)
{
	const voe_scene_identity *rows;
	const voe_ecs_entity *entities;
	uint32_t count;
	uint32_t order[VOE_GAME_WORLD_AUTHORED];
	uint32_t written = 1;

	VOE_BASE_ASSERT(world != NULL, "entity choices out of no world");
	VOE_BASE_ASSERT(out != NULL && room > 0, "entity choices into no room");

	rows = voe_scene_identity_rows(world);
	entities = voe_scene_identity_entities(world);
	count = voe_scene_identity_count(world);
	VOE_BASE_ASSERT(count <= VOE_GAME_WORLD_AUTHORED,
			"more identities than VOE_GAME_WORLD_AUTHORED");

	// Row indices, insertion-sorted by the id each row holds.
	for (uint32_t i = 0; i < count; i++) {
		uint32_t at = i;

		while (at > 0 && rows[order[at - 1]].id > rows[i].id) {
			order[at] = order[at - 1];
			at--;
		}
		order[at] = i;
	}

	out[0] = (voe_ecs_entity){ 0 };
	for (uint32_t i = 0; i < count && written < room; i++)
		out[written++] = entities[order[i]];

	VOE_BASE_ASSERT(written <= room, "entity choices past their room");
	return written;
}

const char *voe_editor_entity_field_label(const voe_ecs_world *world,
					  voe_ecs_entity entity,
					  voe_base_arena *arena)
{
	const voe_scene_identity *identity;

	VOE_BASE_ASSERT(world != NULL, "an entity's label out of no world");
	VOE_BASE_ASSERT(arena != NULL, "an entity's label into no arena");

	if (!voe_ecs_entity_alive(world, entity))
		return NONE_TEXT;
	identity = voe_scene_identity_get(world, entity);
	if (identity == NULL)
		return NONE_TEXT;

	return chars(arena, sizeof identity->name,
		     (const uint8_t *)identity->name);
}
