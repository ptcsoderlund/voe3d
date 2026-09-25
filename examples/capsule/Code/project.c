// The project's two entry points (game/project.h). Teaches where a project
// starts: register names every component the project has, and systems_run
// is the one frame's order, each system's writes before the next reads.
//
// The game links both and calls register once, after the engine's types, and
// systems_run before each frame; the editor loads this code as a library and
// calls only register (0242).
//
// Constraints: keyboard, then player, then follow; the order is the data flow.
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
	(void)follow_camera_register(world);
}

void voe_game_project_systems_run(const voe_game_project_step *step)
{
	VOE_BASE_ASSERT(step != NULL && step->world != NULL,
			"running the project's systems on no world");
	keyboard_system_run(step->world, step->window);
	player_system_run(step->world, step->seconds);
	follow_camera_system_run(step->world);
}
