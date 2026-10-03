// The tank game's scroll: the camera follows the tank forward and knows where
// the screen's edges meet the ground (0334 point 1).
//
//     tank_scroll_register(world);   // in voe_game_project_register
//     tank_scroll_run(step);         // after the move, each fixed step
//     const tank_scroll *s = tank_scroll_get(world);   // NULL before it is made
//
// THE LEVEL RUNS ALONG WORLD -Z, as the scene's camera looks. `lead` is the
// camera's z less the first hull's at the first step with both; after, the
// camera's z is the lesser of its own and the hull's z plus `lead`, so it
// follows the tank forward and never goes back. `bottom` and `top` are the z
// where the view's bottom and top centre rays meet the plane at the hull's
// height, the camera's z less its far_plane for a ray that never meets it:
// what the hull may not back out past, and how far ahead a wave wakes.
//
// tank_scroll is one runtime-only row on the scene's one camera (0261), queued
// through the structural queue, so it is there from the step after. Never
// saved, never in the Inspector, no menu; the sponsor's scene is not edited
// (0272). ONE WRITER, THIS MODULE; the camera moves by transform intent.
//
// It runs after the move (0256), so it follows where the hull went this step
// and the camera is drawn at the same lag as the hull.
//
// Constraints: one row (capacity 1). The camera is the scene's first camera
// and a root; its x, y and rotation are kept. Headless the same: nothing here
// reads the window. A full transform queue leaves the camera for next step.
#pragma once

#include <ecs/world.h>

#include <game/project.h>

#include <stdbool.h>

typedef struct {
	double lead;
	double bottom;
	double top;
} tank_scroll;

extern const struct voe_ecs_key tank_scroll_key;

// Registers tank_scroll runtime-only with no menu. False, reported, when game
// refuses it.
[[nodiscard]] bool tank_scroll_register(voe_ecs_world *world);

// The scroll row, or NULL before its first step with a camera and a hull.
const tank_scroll *tank_scroll_get(const voe_ecs_world *world);

// Queues the row on the first step with a camera and a hull; after, moves the
// camera forward with the hull and writes the edges whole.
void tank_scroll_run(const voe_game_project_step *step);
