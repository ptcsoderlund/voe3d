// The tank game's four entry points (game/project.h). Register names every
// component the game has; each fixed step has two slots, before the bodies'
// move and after it (0256).
//
// systems_run is before the move: the hull, which drives on WASD.
// systems_after_move has nothing yet. interface runs once a frame, after
// the steps (0259): the game draws none, so it ends the ui frame game began
// and returns true.
//
// The game links all four and calls register once, after the engine's
// types, both slots each fixed step and interface each frame; the editor
// loads this code as a library and calls only register (0242).
//
// Constraints: before the move, the hull; after it, nothing. The order is
// the data flow.
#include "tank_hull.h"

#include <base/assert.h>

#include <game/project.h>

void voe_game_project_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering the project in no world");
	// A refusal is game's line on stderr; the system that needs the refused
	// type asserts on its first run.
	(void)tank_hull_register(world);
}

void voe_game_project_systems_run(const voe_game_project_step *step)
{
	VOE_BASE_ASSERT(step != NULL && step->world != NULL,
			"running the project's systems on no world");
	tank_hull_system_run(step->world, step->window, step->seconds);
}

void voe_game_project_systems_after_move(const voe_game_project_step *step)
{
	VOE_BASE_ASSERT(step != NULL && step->world != NULL,
			"running the project's systems on no world");
}

bool voe_game_project_interface(const voe_game_project_frame *frame)
{
	VOE_BASE_ASSERT(frame != NULL && frame->ui != NULL,
			"drawing the project's interface in no frame");
	(void)voe_ui_frame_end(frame->ui);
	return true;
}
