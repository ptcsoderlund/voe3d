// The coin game's four entry points (game/project.h): register names every
// component the game has (rotator, player, player_state, coin, coin_taken
// and game_state), and each fixed step has two slots (0256), each system's
// writes before the next reads.
//
// systems_run is before the bodies' move: game, player, then rotator.
// systems_after_move is after it, in the same step: player_camera. interface runs once a frame,
// after the steps (0259): none yet, so it ends the ui frame game began and
// returns true. Later cards add their types and systems to these lists.
//
// The game links all four and calls register once, after the engine's
// types, both slots each fixed step and interface each frame; the editor
// loads this code as a library and calls only register (0242).
//
// Constraints: the order within a slot is the data flow.
#include "coin.h"
#include "game_state.h"
#include "player.h"
#include "rotator.h"

#include <base/assert.h>

#include <game/project.h>

void voe_game_project_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering the project in no world");
	// A refusal is game's line on stderr; the system that needs the refused
	// type asserts on its first run.
	(void)rotator_register(world);
	(void)player_register(world);
	(void)coin_register(world);
	(void)game_state_register(world);
}

void voe_game_project_systems_run(const voe_game_project_step *step)
{
	VOE_BASE_ASSERT(step != NULL && step->world != NULL,
			"running the project's systems on no world");
	game_system_run(step->world, step->seconds);
	player_system_run(step->world, step->window, step->seconds);
	rotator_system_run(step->world, step->seconds);
}

void voe_game_project_systems_after_move(const voe_game_project_step *step)
{
	VOE_BASE_ASSERT(step != NULL && step->world != NULL,
			"running the project's systems on no world");
	player_camera_run(step->world);
}

bool voe_game_project_interface(const voe_game_project_frame *frame)
{
	VOE_BASE_ASSERT(frame != NULL && frame->ui != NULL,
			"drawing the project's interface in no frame");
	(void)voe_ui_frame_end(frame->ui);
	return true;
}
