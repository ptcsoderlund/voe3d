// The player's registration: its key, its default row and its menu entry.
// The numbers are the sponsor's defaults (0260 point 1).
//
// Constraints: registers only; nothing here runs in a step.
#include "player.h"

#include <base/assert.h>

#include <game/project.h>
#include <game/world.h>

const struct voe_ecs_key player_key = { "player" };

static const player player_default = {
	.speed = 2.0f,
	.jump_height = 1.2f,
	.gravity = 9.81f,
	.camera_distance = 6.0f,
	.start_score = 1000,
	.score_drop = 10,
};

bool player_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering player in no world");
	return voe_game_project_component(world, &(voe_game_project_type){
		&player_key, sizeof(player), VOE_GAME_WORLD_AUTHORED,
		VOE_GAME_PROJECT_DESCRIPTION(player), &player_default,
		"Player" });
}
