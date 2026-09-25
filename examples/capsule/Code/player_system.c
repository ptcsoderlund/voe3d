// The player system: keyboard_input and player into a body intent. Teaches
// walking one table and looking the others up by entity, and that a project
// moves an engine row the way anything outside physics does, by reading it
// and submitting the whole changed body (rule 4); the body's move then slides
// it along what it hits.
//
// On the floor the Y velocity starts at the jump's, √(2 · gravity · height),
// when `jump` is set, else at 0; in the air it starts at the body's own Y,
// which the move left as what it really moved, so a ceiling stops a rise.
// Space in the air does nothing because only the floor branch reads `jump`.
// Gravity is then taken off in both.
//
// Constraints: a full body queue drops the rest of this step's moves.
#include "keyboard_input.h"
#include "player.h"

#include <base/assert.h>

#include <ecs/component.h>

#include <game/project.h>
#include <game/world.h>

#include <physics/body_system.h>

#include <math.h>

const struct voe_ecs_key player_key = { "player" };

static const player player_default = { .speed = 2.0f, .jump_height = 1.2f,
				       .gravity = 9.81f };

bool player_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering player in no world");
	return voe_game_project_component(world, &(voe_game_project_type){
		&player_key, sizeof(player), VOE_GAME_WORLD_AUTHORED,
		VOE_GAME_PROJECT_DESCRIPTION(player), &player_default,
		"Player" });
}

// The Y velocity a player wants this step, before the body's move.
static float rise_of(const player *row, const keyboard_input *input,
		     const voe_physics_body *body, float seconds)
{
	float rise = body->velocity.y;

	if (body->on_floor)
		rise = input->jump ? sqrtf(2.0f * fmaxf(row->gravity, 0.0f) *
					   fmaxf(row->jump_height, 0.0f)) :
				     0.0f;
	return rise - row->gravity * seconds;
}

void player_system_run(voe_ecs_world *world, double seconds)
{
	VOE_BASE_ASSERT(world != NULL, "moving players in no world");
	VOE_BASE_ASSERT(seconds >= 0.0, "moving players back in time");
	const voe_ecs_type type = voe_ecs_component_type(world, &player_key);
	const voe_ecs_type input_type =
		voe_ecs_component_type(world, &keyboard_input_key);
	const player *rows = voe_ecs_component_rows(world, type);
	const voe_ecs_entity *entities = voe_ecs_component_entities(world, type);
	const uint32_t count = voe_ecs_component_count(world, type);

	VOE_BASE_ASSERT(count <= VOE_GAME_WORLD_AUTHORED,
			"more player rows than were registered");
	for (uint32_t i = 0; i < count; i++) {
		const keyboard_input *input =
			voe_ecs_component_get(world, input_type, entities[i]);
		const voe_physics_body *body =
			voe_physics_body_get(world, entities[i]);

		if (input == NULL || body == NULL)
			continue;
		voe_physics_body wanted = *body;

		wanted.velocity.x = input->move.x * rows[i].speed;
		wanted.velocity.z = -input->move.y * rows[i].speed;
		wanted.velocity.y =
			rise_of(&rows[i], input, body, (float)seconds);
		if (!voe_physics_body_submit(world, (voe_physics_body_intent){
							    entities[i], wanted }))
			return;
	}
}
