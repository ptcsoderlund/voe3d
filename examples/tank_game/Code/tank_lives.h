// The player's lives: how many hits the tank has left, and the HUD that
// shows them (0294 point 4).
//
//     tank_lives_register(world);          // in voe_game_project_register
//     tank_lives_run(step);                // after the shells, before the spawner
//     return tank_lives_interface(frame);  // in voe_game_project_interface
//
// tank_lives is one runtime-only row on the first hull: never saved, never in
// the Inspector, no menu. ONE ROW, ONE WRITER, THIS MODULE. tank_lives_run's
// first step with a hull adds it with `lives` 3 through the structural queue,
// headless too, so it is read from the step after.
//
// WHAT COSTS ONE: each tank_shot this step with `hit` whose `target` is the
// row's hull or anything under it. The count never goes below 0. At 0
// nothing happens yet; what 0 does is milestone 14's.
//
// The enemies (08) read the row for whom to fire at: its entity is the
// player's hull.
//
// The interface draws a small panel at the top left reading `Lives N`, then
// ends the ui frame and returns true; with no row it only ends the frame.
//
// Constraints: one row (capacity 1). A full structural queue tries again next
// step.
#pragma once

#include <ecs/world.h>

#include <game/project.h>

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	int32_t lives;
} tank_lives;

extern const struct voe_ecs_key tank_lives_key;

// Registers tank_lives runtime-only with no menu. False, reported, when game
// refuses it.
[[nodiscard]] bool tank_lives_register(voe_ecs_world *world);

// Adds the row of 3 to the first hull on the first step with one; after,
// takes one off for each shot that hit that hull or under it this step.
void tank_lives_run(const voe_game_project_step *step);

// Draws the lives panel, ends the ui frame and returns true.
bool tank_lives_interface(const voe_game_project_frame *frame);
