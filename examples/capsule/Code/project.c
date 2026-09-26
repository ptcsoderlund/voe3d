// The project's three entry points (game/project.h). Teaches where a project
// starts: register names every component the project has, and each fixed
// step has two slots, each system's writes before the next reads.
//
// systems_run is before the bodies' move: keyboard, player and coin, which
// set what the bodies do. systems_after_move is after it, in the same step:
// follow, because a camera that reads its target before the move is drawn a
// step behind it (bug 01, 0257).
//
// The game links all three and calls register once, after the engine's
// types, and both slots each fixed step; the editor loads this code as a
// library and calls only register (0242).
//
// Constraints: before the move, keyboard, then player, then coin; after the
// move, follow. The order is the data flow.
#include "coin.h"
#include "follow_camera.h"
#include "keyboard_input.h"
#include "player.h"

#include <base/assert.h>

#include <game/project.h>

void voe_game_project_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering the project in no world");
	// A refusal is game's line on stderr and the rest still registers; the
	// system that needs the refused type asserts on its first run.
	(void)keyboard_input_register(world);
	(void)player_register(world);
	(void)coin_register(world);
	(void)follow_camera_register(world);
}

void voe_game_project_systems_run(const voe_game_project_step *step)
{
	VOE_BASE_ASSERT(step != NULL && step->world != NULL,
			"running the project's systems on no world");
	keyboard_system_run(step->world, step->window);
	player_system_run(step->world, step->seconds);
	coin_system_run(step->world, step->seconds);
}

void voe_game_project_systems_after_move(const voe_game_project_step *step)
{
	VOE_BASE_ASSERT(step != NULL && step->world != NULL,
			"running the project's systems on no world");
	follow_camera_system_run(step->world);
}
