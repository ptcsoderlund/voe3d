// The player's lives: how many hits the tank has left (0294 point 4); the
// menu's HUD shows them (tank_menu.c, 0334 point 7).
//
//     tank_lives_register(world);          // in voe_game_project_register
//     tank_lives_run(step);                // after the shells, before the spawner
//
// tank_lives is one runtime-only row on the first hull: never saved, never in
// the Inspector, no menu. ONE ROW, ONE WRITER, THIS MODULE. tank_lives_run's
// first step with a hull adds it with `lives` 3 through the structural queue,
// headless too, so it is read from the step after.
//
// WHAT COSTS ONE: each tank_shot this step with `hit` whose `target` is the
// row's hull or anything under it. The count never goes below 0. At 0 the
// state sets the round lost.
//
// The enemies (08) read the row for whom to fire at: its entity is the
// player's hull.
//
// THE HULL IS MADE SOLID (0295): tank_lives_run queues a box collider of
// (4.64, 4, 2.62), the enemy's size, not a trigger, onto the first hull each
// step it has none, so enemy shells hit it. A collider the sponsor puts on the
// hull is kept and wins. Added only in the running game, which is a separate
// program, so it is never saved.
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
