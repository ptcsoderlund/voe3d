// The tank goal's key and registration. Whether the player reached it is the
// state system's; this file only makes the type and names its need of a
// transform (0303).
//
// Constraints: none past tank_goal.h's.
#include "tank_goal.h"

#include <base/assert.h>

#include <game/project.h>
#include <game/world.h>

#include <scene/transform_component.h>

const struct voe_ecs_key tank_goal_key = { "tank_goal" };

static const tank_goal tank_goal_default = { .points = 1000 };

bool tank_goal_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering tank goal in no world");
	return voe_game_project_component(world, &(voe_game_project_type){
		&tank_goal_key, sizeof(tank_goal), VOE_GAME_WORLD_AUTHORED,
		VOE_GAME_PROJECT_DESCRIPTION(tank_goal),
		&tank_goal_default, "Tank / Goal" }) &&
	       voe_game_project_component_needs(world, &tank_goal_key,
						&voe_scene_transform_key);
}
