// The round's state: which phase the game is in, the score so far, and the
// screens' chosen item and last frame's input levels (0334 point 6).
//
//     tank_state_register(world);          // in voe_game_project_register
//     const tank_state *s = tank_state_get(world);   // NULL before it is made
//     tank_state_run(step);                // after the lives, before the spawner
//     return tank_menu_run(frame);         // in voe_game_project_interface
//
// A FRESH WORLD STARTS AT THE MENU. tank_state_run's first step with a hull
// queues the row (menu, 0) onto the first hull through the structural queue,
// so it is read from the step after; before that, and with no row, the game is
// not playing.
//
// While playing, each step adds the `points` of every tank_shot with `hit`,
// sets lost when the lives row is at 0, and else, when the hull's z is at or
// below the first goal's world z, adds the goal's `points` and sets won. In
// any other phase the step changes nothing: the screens move the phase.
// tank_menu_run draws the screens and answers them (tank_menu.c).
//
// tank_state is one runtime-only row on the first hull: never saved, never in
// the Inspector, no menu. ONE ROW, ONE WRITER, THIS MODULE: the step in
// tank_state_system.c and the screens in tank_menu.c.
//
// Constraints: one row (capacity 1). A full structural queue tries again next
// step.
#pragma once

#include <ecs/world.h>

#include <game/project.h>

#include <stdbool.h>
#include <stdint.h>

enum {
	TANK_PHASE_MENU,
	TANK_PHASE_PLAYING,
	TANK_PHASE_PAUSED,
	TANK_PHASE_WON,
	TANK_PHASE_LOST,
};

// `selected` is the screen's chosen item; `held` last frame's levels, a bit
// each: up, down, press, pause, back.
typedef struct {
	uint32_t phase;
	int32_t score;
	uint32_t selected;
	uint32_t held;
} tank_state;

extern const struct voe_ecs_key tank_state_key;

// Registers tank_state runtime-only with no menu. False, reported, when game
// refuses it.
[[nodiscard]] bool tank_state_register(voe_ecs_world *world);

// The state row, or NULL before its first step with a hull.
const tank_state *tank_state_get(const voe_ecs_world *world);

// Queues the row on the first step with a hull; after, while playing, adds
// this step's shot points and sets lost at 0 lives or won at the goal.
void tank_state_run(const voe_game_project_step *step);

// Draws the phase's screen, ends the ui frame, answers its keys and clicks
// and sets the run's asks. False when Quit was pressed.
bool tank_menu_run(const voe_game_project_frame *frame);
