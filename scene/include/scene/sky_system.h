// The only way a sky changes: an intent, drained by this system. It needs no
// transform; its sun is the world's first directional light.
//
//     voe_scene_sky_register(world, 1);                     // once, at startup
//     voe_scene_sky_add(world, settings, (voe_scene_sky){
//             .light = 1.0f, .haze = 1000.0f, .tone = VOE_SCENE_SKY_TONE_SOFT });
//
//     // from anywhere: the Inspector, game code
//     voe_scene_sky_submit(world, (voe_scene_sky_intent){
//             .entity = settings, .sky = brighter });
//
//     // once a frame, in the frame loop, before the draw
//     voe_scene_sky_system_run(world);
//
// THE REPLACE IS QUEUED AND NOT WRITTEN, because the table is this system's
// and no one else writes it (rule 4): a submitter reads the row, changes it and
// submits the whole of it, and two in one frame resolve last-writer-wins. It is
// the row's replace, so the Inspector edits it live.
//
// CREATION IS A DIRECT CALL AND NOT AN INTENT, as for every row in this
// folder: an intent is applied to a component that exists.
//
// A NUMBER NOT FINITE, A LIGHT OR HAZE BELOW NOUGHT, AN EXPOSURE PAST ±16 OR A
// TONE PAST SOFT IS REFUSED, not clamped: none has a nearest right answer.
// `add` asserts on one; the drain keeps the row and says so on stderr, one
// line per refused intent. An intent naming an entity with no sky, a
// destroyed one included, is dropped silently.
//
// IT SITS AT "Rendering / Sky" IN ADD COMPONENT, a menu path registered with
// the type (ecs/component.h, 0221).
#pragma once

#include <ecs/world.h>
#include <scene/sky_component.h>

#include <stdint.h>

// Registers the table, the intent queue as its replace and the default row:
// light 1, haze 1000, exposure 0, tone Soft. No needed type. Call it once per
// world, before anything adds a sky. capacity is how many skies the world may
// hold (the game's is 1) and also how many intents may be waiting at once.
void voe_scene_sky_register(voe_ecs_world *world, uint32_t capacity);

// Gives the entity its sky, as given; a row the drain would refuse asserts.
// False when the table is full or the entity is not alive.
[[nodiscard]] bool voe_scene_sky_add(voe_ecs_world *world,
				     voe_ecs_entity entity, voe_scene_sky sky);

// Make this entity's sky the one the submitter says.
typedef struct {
	voe_ecs_entity entity;
	voe_scene_sky sky;
} voe_scene_sky_intent;

// False when the queue is full — the system has not run for long enough.
[[nodiscard]] bool voe_scene_sky_submit(voe_ecs_world *world,
					voe_scene_sky_intent intent);

// Applies every waiting intent, in submission order, and empties the queue.
void voe_scene_sky_system_run(voe_ecs_world *world);
