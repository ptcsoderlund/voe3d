// The tank game's four entry points (game/project.h). Register names every
// component the game has; each fixed step has two slots, before the bodies'
// move and after it (0256).
//
// systems_run is before the move: the control first, which reads the pad
// beside the keyboard and mouse into one row, the last touched winning; then
// the hull, which drives on the row, then the turret, which aims at the
// pointer or the right stick, then the gun, which fires while the row's fire
// is held, then the shells, which fly, hit and run out, then the spawner,
// which makes enemies on a timer, then the enemies, which drive and run out,
// then the camera, which fits its lens to the window's shape (0291).
// The gun is after the turret so it fires along this step's aim. The
// breakable has no system: the shells swap a hit one for its wreck.
// systems_after_move has nothing yet. interface runs once a frame, after
// the steps (0259): the game draws none, so it ends the ui frame game began
// and returns true.
//
// The game links all four and calls register once, after the engine's
// types, both slots each fixed step and interface each frame; the editor
// loads this code as a library and calls only register (0242).
//
// Constraints: before the move, the control, hull, turret, gun, shell, spawner, enemy,
// then camera; after it, nothing. The order is the data flow.
#include "tank_breakable.h"
#include "tank_camera.h"
#include "tank_control.h"
#include "tank_enemy.h"
#include "tank_gun.h"
#include "tank_hull.h"
#include "tank_shell.h"
#include "tank_spawner.h"
#include "tank_turret.h"

#include <base/assert.h>

#include <game/project.h>

void voe_game_project_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering the project in no world");
	// A refusal is game's line on stderr; the system that needs the refused
	// type asserts on its first run.
	(void)tank_hull_register(world);
	(void)tank_control_register(world);
	(void)tank_turret_register(world);
	(void)tank_gun_register(world);
	(void)tank_shell_register(world);
	(void)tank_breakable_register(world);
	(void)tank_enemy_register(world);
	(void)tank_spawner_register(world);
	(void)tank_camera_register(world);
}

void voe_game_project_systems_run(const voe_game_project_step *step)
{
	VOE_BASE_ASSERT(step != NULL && step->world != NULL,
			"running the project's systems on no world");
	tank_control_run(step);
	tank_hull_system_run(step->world, step->seconds);
	tank_turret_system_run(step->world, step->window, step->seconds);
	tank_gun_system_run(step);
	tank_shell_system_run(step);
	tank_spawner_system_run(step);
	tank_enemy_system_run(step);
	tank_camera_run(step);
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
