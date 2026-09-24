// The player system: keyboard_input and player into a transform intent.
// Teaches walking one table and looking the others up by entity, and that a
// project moves an engine row the way anything outside scene does, by
// reading it and submitting the whole changed transform (rule 4).
//
// Constraints: moves on XZ only, with no collision; a full transform queue
// drops the rest of this frame's moves.
#include "keyboard_input.h"
#include "player.h"

#include <base/assert.h>

#include <ecs/component.h>

#include <game/project.h>
#include <game/world.h>

#include <scene/transform_system.h>

const struct voe_ecs_key player_key = { "player" };

static const player player_default = { .speed = 2.0f };

bool player_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering player in no world");
	return voe_game_project_component(world, &(voe_game_project_type){
		&player_key, sizeof(player), VOE_GAME_WORLD_AUTHORED,
		VOE_GAME_PROJECT_DESCRIPTION(player), &player_default,
		"Player" });
}

void player_system_run(voe_ecs_world *world, double seconds)
{
	VOE_BASE_ASSERT(world != NULL, "moving players in no world");
	VOE_BASE_ASSERT(seconds >= 0.0, "moving players back in time");
	const voe_ecs_type type = voe_ecs_component_type(world, &player_key);
	const voe_ecs_type input_type =
		voe_ecs_component_type(world, &keyboard_input_key);
	const voe_ecs_type transform_type =
		voe_ecs_component_type(world, &voe_scene_transform_key);
	const player *rows = voe_ecs_component_rows(world, type);
	const voe_ecs_entity *entities = voe_ecs_component_entities(world, type);
	const uint32_t count = voe_ecs_component_count(world, type);

	VOE_BASE_ASSERT(count <= VOE_GAME_WORLD_AUTHORED,
			"more player rows than were registered");
	for (uint32_t i = 0; i < count; i++) {
		const keyboard_input *input =
			voe_ecs_component_get(world, input_type, entities[i]);
		const voe_scene_transform *transform = voe_ecs_component_get(
			world, transform_type, entities[i]);

		if (input == NULL || transform == NULL)
			continue;
		const float step = rows[i].speed * (float)seconds;
		voe_scene_transform moved = *transform;

		moved.position.x += input->move.x * step;
		moved.position.z -= input->move.y * step;
		if (!voe_scene_transform_submit(world,
						(voe_scene_transform_intent){
							entities[i], moved }))
			return;
	}
}
