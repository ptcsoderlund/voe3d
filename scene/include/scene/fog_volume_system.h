// The only way a fog volume or its drift changes: an intent, drained by this
// system, and the run that moves the drift. Where the box is, how it is
// turned and scaled are its transform's, and change through transform_system.h.
//
//     voe_scene_transform_register(world, 64);              // once, at startup
//     voe_scene_fog_volume_register(world, 8);
//     voe_scene_fog_volume_add(world, valley, (voe_scene_fog_volume){
//             .size = { 40.0f, 10.0f, 40.0f }, .thickness = 0.4f, ... });
//
//     // from anywhere: the Inspector, game code
//     voe_scene_fog_volume_submit(world, (voe_scene_fog_volume_intent){
//             .entity = valley, .fog_volume = thicker });
//
//     voe_scene_fog_volume_system_run(world, seconds);      // each step
//
// IT RUNS WHERE THE CLOUDS' RUN DOES (scene/clouds_system.h), the editor's
// world step and the game's fixed step, so fog drifts alike in edit, Play and
// a game.
//
// A RUN, IN ORDER:
// - it drains the replaces in submission order: an entity with no fog volume,
//   a destroyed one included, is dropped silently;
// - it adds a drift at 0 to each row lacking one and drops the drifts whose
//   row is gone;
// - it moves each drift by wind × seconds, in double.
//
// REFUSED, NOT CLAMPED: a number not finite, a size component below 0,
// thickness, patchiness or godrays strength outside 0 to 1, wind speed below
// 0. `add` asserts on one; the drain keeps the row and says so on stderr, one
// line per refused intent.
//
// IT SITS AT "Rendering / Fog volume" IN ADD COMPONENT, a menu path
// registered with the type (ecs/component.h, 0221).
#pragma once

#include <ecs/world.h>
#include <scene/fog_volume_component.h>

#include <stdint.h>

// Registers the table, the intent queue as its replace, the transform it needs
// and the default row of 0493 point 1 (size 40, 10, 40, thickness 0.4,
// patchiness 0.6, wind 0 at 1, godrays 1), which is also the unsaid row; and
// the drift table, runtime-only. Call it after the transform table (it asserts
// on none). capacity is how many volumes the world may hold, how many drifts,
// and how many intents may wait at once.
void voe_scene_fog_volume_register(voe_ecs_world *world, uint32_t capacity);

// Gives the entity its fog volume, as given; a row the drain would refuse
// asserts. False when the table is full or the entity is not alive.
[[nodiscard]] bool voe_scene_fog_volume_add(voe_ecs_world *world,
					    voe_ecs_entity entity,
					    voe_scene_fog_volume fog_volume);

// Make this entity's fog volume the one the submitter says.
typedef struct {
	voe_ecs_entity entity;
	voe_scene_fog_volume fog_volume;
} voe_scene_fog_volume_intent;

// False when the queue is full — the system has not run for long enough.
[[nodiscard]] bool
voe_scene_fog_volume_submit(voe_ecs_world *world,
			    voe_scene_fog_volume_intent intent);

// Drains, adds and drops drifts, and moves each `seconds` on. Asserts
// seconds >= 0.
void voe_scene_fog_volume_system_run(voe_ecs_world *world, float seconds);
