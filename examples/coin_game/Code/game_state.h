// The run's state: which phase the game is in and what the score is made of.
//
//     game_state_register(world);               // in voe_game_project_register
//     game_system_run(world, seconds);          // first, before the move
//     const game_state *state = game_state_get(world);
//     int32_t score = game_score(world);
//     bool going_on = game_interface_run(frame);  // once a frame
//
// game_state is one runtime-only row on the first player entity (0260 point
// 2), added by game_system_run's first step through the structural queue, so
// it exists from the step after. Never saved, never in the Inspector.
//
// ONE OWNER, THIS MODULE, WRITES THE ROW: its step system (game_system.c)
// and its interface (game_interface.c). Everyone else reads it through
// game_state_get.
//
// THE SCORE IS DERIVED, NOT STORED: start_score - dropped + the points of
// coins whose coin_taken carries the current restart count, at least 0. A
// take is a coin_taken row, so taking a coin needs no message to this module.
//
// Constraints: one row; a walk of every coin per score, bounded by
// VOE_GAME_WORLD_AUTHORED, which a running total would lift.
#pragma once

#include <ecs/world.h>

#include <game/project.h>

#include <stdbool.h>
#include <stdint.h>

enum {
	GAME_PHASE_MENU,
	GAME_PHASE_PLAYING,
	GAME_PHASE_WON,
	GAME_PHASE_LOST,
};

// `dropped` is the points lost to time, `banked` the part-second not yet
// counted; the two _down are Enter's and Escape's levels last frame.
typedef struct {
	uint32_t phase;
	int32_t dropped;
	double banked;
	uint32_t restarts;
	bool enter_down;
	bool escape_down;
} game_state;

extern const struct voe_ecs_key game_state_key;

// Registers game_state runtime-only with no menu. False, reported, when game
// refuses it.
[[nodiscard]] bool game_state_register(voe_ecs_world *world);

// The one row, NULL before it is made.
const game_state *game_state_get(const voe_ecs_world *world);

// The current score, never below 0.
int32_t game_score(const voe_ecs_world *world);

// Coins not taken since the last restart.
uint32_t game_coins_left(const voe_ecs_world *world);

// Makes the row on its first run; while playing, drops score_drop a whole
// second and ends the run lost at score 0 or won with no coins left.
void game_system_run(voe_ecs_world *world, double seconds);

// Draws the phase's screen and ends the ui frame; with no row yet only ends
// it. Enter and Escape and the buttons move the phase; false is Quit.
bool game_interface_run(const voe_game_project_frame *frame);
