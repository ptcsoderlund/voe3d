// The only way a light blocker changes: an intent, drained by this system.
// Where it is, how it is turned and scaled are its transform's, and change
// through transform_system.h.
//
//     voe_scene_transform_register(world, 64);              // once, at startup
//     voe_scene_light_blocker_register(world, 32);
//     voe_scene_light_blocker_add(world, house, (voe_scene_light_blocker){
//             .size = { 8.0f, 3.0f, 6.0f } });
//
//     // from anywhere: the Inspector, game code
//     voe_scene_light_blocker_submit(world, (voe_scene_light_blocker_intent){
//             .entity = house, .blocker = bigger });
//
//     // once a frame, in the frame loop, before the draw
//     voe_scene_light_blocker_system_run(world);
//
// THE REPLACE IS QUEUED AND NOT WRITTEN, because the table is this system's
// and no one else writes it (rule 4): a submitter reads the row, changes it and
// submits the whole of it, and two in one frame resolve last-writer-wins. It is
// the row's replace, so the Inspector edits it live.
//
// CREATION IS A DIRECT CALL AND NOT AN INTENT, as for every row in this
// folder: an intent is applied to a component that exists.
//
// A SIZE WITH A COMPONENT NOT FINITE OR BELOW NOUGHT, OR A BLOCK PAST DIRECT, IS
// REFUSED, not clamped: neither has a nearest right answer. `add` asserts on
// one; the drain keeps the
// row and says so on stderr, one line per refused intent. An intent naming an
// entity with no blocker, a destroyed one included, is dropped silently.
//
// IT SITS AT "Rendering / Light blocker" IN ADD COMPONENT, a menu path
// registered with the type (ecs/component.h, 0221).
#pragma once

#include <ecs/world.h>
#include <scene/light_blocker_component.h>

#include <stdint.h>

// Registers the table, the intent queue as its replace, the transform it needs
// and the default row, size (1, 1, 1) and Block All, and sets the former name
// "kind" for "block" (0353). Call it once per world, after the
// transform table (it asserts on none) and before anything adds a blocker.
// capacity is how many blockers the world may hold and also how many intents
// may be waiting at once.
void voe_scene_light_blocker_register(voe_ecs_world *world, uint32_t capacity);

// Gives the entity its blocker, as given; a size or block the drain would refuse
// asserts. False when the table is full or the entity is not alive.
[[nodiscard]] bool voe_scene_light_blocker_add(voe_ecs_world *world,
					       voe_ecs_entity entity,
					       voe_scene_light_blocker blocker);

// Make this entity's blocker the one the submitter says.
typedef struct {
	voe_ecs_entity entity;
	voe_scene_light_blocker blocker;
} voe_scene_light_blocker_intent;

// False when the queue is full — the system has not run for long enough.
[[nodiscard]] bool
voe_scene_light_blocker_submit(voe_ecs_world *world,
			       voe_scene_light_blocker_intent intent);

// Applies every waiting intent, in submission order, and empties the queue.
void voe_scene_light_blocker_system_run(voe_ecs_world *world);
