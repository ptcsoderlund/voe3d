// The only way a haze row changes: an intent, drained by this system. It needs
// no transform; it belongs to the world, not to a place in it.
//
//     voe_scene_haze_register(world, 1);                    // once, at startup
//     voe_scene_haze_add(world, settings, (voe_scene_haze){
//             .haze = 1000.0f, .start = 100.0f, .height = 1000.0f });
//
//     // from anywhere: the Inspector, game code
//     voe_scene_haze_submit(world, (voe_scene_haze_intent){
//             .entity = settings, .haze = thicker });
//
//     // once a frame, in the frame loop, before the draw
//     voe_scene_haze_system_run(world);
//
// THE REPLACE IS QUEUED AND NOT WRITTEN, because the table is this system's
// and no one else writes it (rule 4): a submitter reads the row, changes it and
// submits the whole of it, and two in one frame resolve last-writer-wins. It is
// the row's replace, so the Inspector edits it live.
//
// CREATION IS A DIRECT CALL AND NOT AN INTENT, as for every row in this
// folder: an intent is applied to a component that exists.
//
// A HAZE OR START NOT FINITE OR BELOW 0, OR A HEIGHT NOT FINITE, IS REFUSED,
// not clamped: none has a nearest right answer. Any finite height is taken.
// `add` asserts on one; the drain keeps the row and says so on stderr, one
// line per refused intent. An intent naming an entity with
// no haze, a destroyed one included, is dropped silently.
//
// IT SITS AT "Rendering / Haze" IN ADD COMPONENT, a menu path registered with
// the type (ecs/component.h, 0221).
#pragma once

#include <ecs/world.h>
#include <scene/haze_component.h>

#include <stdint.h>

// Registers the table, the intent queue as its replace and the default row of
// 0417 and 0418: haze 1000, start 100, height 1000. It is also the unsaid row,
// what a file that does not mention a field means, so none is set apart. No
// needed type. Call it once per world, before anything adds a haze. capacity is how many hazes the world
// may hold (the game's is 1) and also how many intents may be waiting at once.
void voe_scene_haze_register(voe_ecs_world *world, uint32_t capacity);

// Gives the entity its haze, as given; a row the drain would refuse asserts.
// False when the table is full or the entity is not alive.
[[nodiscard]] bool voe_scene_haze_add(voe_ecs_world *world,
				      voe_ecs_entity entity,
				      voe_scene_haze haze);

// Make this entity's haze the one the submitter says.
typedef struct {
	voe_ecs_entity entity;
	voe_scene_haze haze;
} voe_scene_haze_intent;

// False when the queue is full — the system has not run for long enough.
[[nodiscard]] bool voe_scene_haze_submit(voe_ecs_world *world,
					 voe_scene_haze_intent intent);

// Applies every waiting intent, in submission order, and empties the queue.
void voe_scene_haze_system_run(voe_ecs_world *world);
