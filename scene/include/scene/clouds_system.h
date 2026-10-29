// The only way a clouds row or its drift changes: an intent, drained by this
// system, and the run that moves the drift. It needs no transform; it belongs
// to the world, not to a place in it.
//
//     voe_scene_clouds_register(world, 1);                  // once, at startup
//     voe_scene_clouds_add(world, settings, (voe_scene_clouds){
//             .coverage = 0.5f, .height = 1500.0f, .wind_speed = 10.0f });
//
//     // from anywhere: the Inspector, game code
//     voe_scene_clouds_submit(world, (voe_scene_clouds_intent){
//             .entity = settings, .clouds = denser });
//
//     voe_scene_clouds_system_run(world, seconds);          // each step
//
// IT RUNS WHERE WATER'S RUN DOES (3d/water_system.h), the editor's world step
// and the game's fixed step, so clouds drift alike in edit, Play and a game.
//
// A RUN, IN ORDER:
// - it drains the replaces in submission order: an entity with no clouds, a
//   destroyed one included, is dropped silently;
// - it adds a drift at 0 to each row lacking one and drops the drifts whose
//   row is gone;
// - it moves each drift by wind × seconds, in double.
//
// REFUSED, NOT CLAMPED: a number not finite, a kind past Volumetric, coverage
// or shadow strength outside 0 to 1, thickness or wind speed below 0. `add`
// asserts on one; the drain keeps the row and says so on stderr, one line per
// refused intent.
//
// IT SITS AT "Rendering / Clouds" IN ADD COMPONENT, a menu path registered with
// the type (ecs/component.h, 0221).
#pragma once

#include <ecs/world.h>
#include <scene/clouds_component.h>

#include <stdint.h>

// Registers the table, the intent queue as its replace, the default row of
// 0492 point 1 (Flat, coverage 0.5, height 1500, thickness 600, wind 0 at 10,
// shadow 0.6), which is also the unsaid row, and no needed type; and the drift
// table, runtime-only. capacity is how many clouds the world may hold (the
// game's is 1), how many drifts, and how many intents may wait at once.
void voe_scene_clouds_register(voe_ecs_world *world, uint32_t capacity);

// Gives the entity its clouds, as given; a row the drain would refuse asserts.
// False when the table is full or the entity is not alive.
[[nodiscard]] bool voe_scene_clouds_add(voe_ecs_world *world,
					voe_ecs_entity entity,
					voe_scene_clouds clouds);

// Make this entity's clouds the ones the submitter says.
typedef struct {
	voe_ecs_entity entity;
	voe_scene_clouds clouds;
} voe_scene_clouds_intent;

// False when the queue is full — the system has not run for long enough.
[[nodiscard]] bool voe_scene_clouds_submit(voe_ecs_world *world,
					   voe_scene_clouds_intent intent);

// Drains, adds and drops drifts, and moves each `seconds` on. Asserts
// seconds >= 0.
void voe_scene_clouds_system_run(voe_ecs_world *world, float seconds);
