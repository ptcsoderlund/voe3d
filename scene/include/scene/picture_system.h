// The only way a picture row changes: an intent, drained by this system. It
// needs no transform; it belongs to the world, not to a place in it.
//
//     voe_scene_picture_register(world, 1);                 // once, at startup
//     voe_scene_picture_add(world, settings, (voe_scene_picture){
//             .exposure = 0.0f, .tone = VOE_SCENE_PICTURE_TONE_SOFT });
//
//     // from anywhere: the Inspector, game code
//     voe_scene_picture_submit(world, (voe_scene_picture_intent){
//             .entity = settings, .picture = brighter });
//
//     // once a frame, in the frame loop, before the draw
//     voe_scene_picture_system_run(world);
//
// THE REPLACE IS QUEUED AND NOT WRITTEN, because the table is this system's
// and no one else writes it (rule 4): a submitter reads the row, changes it and
// submits the whole of it, and two in one frame resolve last-writer-wins. It is
// the row's replace, so the Inspector edits it live.
//
// CREATION IS A DIRECT CALL AND NOT AN INTENT, as for every row in this
// folder: an intent is applied to a component that exists.
//
// AN EXPOSURE NOT FINITE OR PAST 16 STOPS EITHER WAY, OR A TONE PAST SOFT, IS
// REFUSED, not clamped: none has a nearest right answer. `add` asserts on one;
// the drain keeps the row and says so on stderr, one line per refused intent.
// An intent naming an entity with no picture, a destroyed one included, is
// dropped silently.
//
// IT SITS AT "Rendering / Picture" IN ADD COMPONENT, a menu path registered
// with the type (ecs/component.h, 0221).
#pragma once

#include <ecs/world.h>
#include <scene/picture_component.h>

#include <stdint.h>

// Registers the table, the intent queue as its replace and the default row of
// 0406: exposure 0, tone Soft. No needed type. Call it once per world, before
// anything adds a picture. capacity is how many pictures the world may hold
// (the game's is 1) and also how many intents may be waiting at once.
void voe_scene_picture_register(voe_ecs_world *world, uint32_t capacity);

// Gives the entity its picture, as given; a row the drain would refuse
// asserts. False when the table is full or the entity is not alive.
[[nodiscard]] bool voe_scene_picture_add(voe_ecs_world *world,
					 voe_ecs_entity entity,
					 voe_scene_picture picture);

// Make this entity's picture the one the submitter says.
typedef struct {
	voe_ecs_entity entity;
	voe_scene_picture picture;
} voe_scene_picture_intent;

// False when the queue is full — the system has not run for long enough.
[[nodiscard]] bool voe_scene_picture_submit(voe_ecs_world *world,
					    voe_scene_picture_intent intent);

// Applies every waiting intent, in submission order, and empties the queue.
void voe_scene_picture_system_run(voe_ecs_world *world);
