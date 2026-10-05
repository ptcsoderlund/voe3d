// The project's four entry points (game/project.h). Teaches the least a
// project needs to change the scene while playing: register names its one
// component, and the step's first slot runs its one system.
//
// systems_run is before the bodies' move: day night, which moves each clock
// on by the step's seconds and submits the lights it changes.
// systems_after_move runs nothing; nothing here follows a body.
//
// interface runs once a frame, after the steps (0259): the example draws
// none, so it ends the ui frame game began and returns true.
//
// The game links all four and calls register once, after the engine's
// types, both slots each fixed step and interface each frame; the editor
// loads this code as a library and calls only register (0242).
//
// Constraints: one system, before the move.
#include "day_night.h"

#include <base/assert.h>

#include <game/project.h>

void voe_game_project_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering the project in no world");
	// A refusal is game's line on stderr; the system asserts on its first
	// run.
	(void)day_night_register(world);
}

void voe_game_project_systems_run(const voe_game_project_step *step)
{
	VOE_BASE_ASSERT(step != NULL && step->world != NULL,
			"running the project's systems on no world");
	day_night_system_run(step->world, step->seconds);
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
