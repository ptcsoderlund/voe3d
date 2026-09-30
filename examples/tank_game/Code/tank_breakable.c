// The tank breakable's key and registration. What a hit does to it is the
// shell system's (tank_shell_system.c); this file only makes the type.
//
// Constraints: none past tank_breakable.h's.
#include "tank_breakable.h"

#include <base/assert.h>

#include <game/project.h>
#include <game/world.h>

const struct voe_ecs_key tank_breakable_key = { "tank_breakable" };

static const tank_breakable tank_breakable_default = { .wreck = "" };

bool tank_breakable_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering tank breakable in no world");
	return voe_game_project_component(world, &(voe_game_project_type){
		&tank_breakable_key, sizeof(tank_breakable),
		VOE_GAME_WORLD_AUTHORED,
		VOE_GAME_PROJECT_DESCRIPTION(tank_breakable),
		&tank_breakable_default, "Tank / Breakable" });
}
