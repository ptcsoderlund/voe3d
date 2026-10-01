// The water system: the one place water and waves rows change (0305 point 5).
// It runs once a frame where the emitters run (3d/emitter_system.h).
//
//     voe_scene_transform_register(world, capacity);
//     voe_3d_water_register(world, capacity);
//     ...
//     voe_3d_water_system_run(world, seconds);              // each frame
//
// A RUN, IN ORDER:
// - it drains the replace intents in submission order: a dead entity or one
//   with no water is dropped;
// - it adds a waves row, at 0 seconds, to each water lacking one and drops the
//   rows whose water is gone, so a removed water's row is gone after the next
//   run;
// - it advances each clock by `seconds`, in double, wrapped below 60, where
//   every wave's phase wraps (0305 point 3);
// - `seconds` of 0 only drains, adds and drops.
//
// Constraints: a replace for an entity with no water is dropped silently.
#pragma once

#include <3d/water_component.h>

#include <ecs/world.h>

// Drains the replaces, adds and drops waves rows and steps every clock
// `seconds` on. Asserts seconds >= 0 and the water registered.
void voe_3d_water_system_run(voe_ecs_world *world, float seconds);
