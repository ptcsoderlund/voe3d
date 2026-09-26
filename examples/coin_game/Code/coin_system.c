// The coin's registration: coin with its default and menu, and coin_taken,
// runtime-only with no menu.
//
// Constraints: registers only; nothing here runs in a step. The runtime-only
// marker is taken by address at run time, because a project library imports
// it on Windows (0245).
#include "coin.h"

#include <base/assert.h>

#include <ecs/component.h>

#include <game/project.h>
#include <game/world.h>

const struct voe_ecs_key coin_key = { "coin" };
const struct voe_ecs_key coin_taken_key = { "coin_taken" };

static const coin coin_default = { .points = 100 };

bool coin_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering coin in no world");
	const bool coin_ok = voe_game_project_component(world,
		&(voe_game_project_type){
			&coin_key, sizeof(coin), VOE_GAME_WORLD_AUTHORED,
			VOE_GAME_PROJECT_DESCRIPTION(coin), &coin_default,
			"Coin" });
	const bool taken_ok = voe_game_project_component(world,
		&(voe_game_project_type){
			&coin_taken_key, sizeof(coin_taken),
			VOE_GAME_WORLD_AUTHORED, &voe_ecs_runtime_only, NULL,
			NULL });

	return coin_ok && taken_ok;
}
