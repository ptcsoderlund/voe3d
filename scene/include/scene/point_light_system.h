// The only way a point light changes: one intent, its replace, drained by this
// system. Where it shines from is its transform's, and changes through
// transform_system.h.
//
//     voe_scene_transform_register(world, 256);             // once, at startup
//     voe_scene_point_light_register(world, 256);
//     voe_scene_point_light_add(world, lamp, (voe_scene_point_light){
//             .colour = { 1.0f, 0.6f, 0.3f }, .intensity = 2.0f,
//             .range = 4.0f, .falloff = 1.0f });
//
//     // from anywhere: a gun's fade, a key, the Inspector
//     dimmer = *voe_scene_point_light_get(world, lamp);
//     dimmer.intensity = 0.5f;
//     voe_scene_point_light_submit(world, (voe_scene_point_light_intent){
//             .entity = lamp, .light = dimmer });
//
//     // once a frame or step
//     voe_scene_point_light_system_run(world);
//
// THE REPLACE CARRIES THE WHOLE LIGHT AND NOT A DELTA, for the reason the sun's
// does: two submitters in one frame resolve as last-writer-wins. It is the
// light's replace, so the Inspector edits it live.
//
// NOTHING IS NORMALIZED OR CORRECTED; A BAD LIGHT IS REFUSED. A number that is
// not finite, a colour channel outside [0, 1], a negative intensity, a range of
// nought or less, a falloff outside LEAST..MOST, bounces past
// VOE_SCENE_LIGHT_BOUNCES_MAX or a negative bounce strength has no nearest
// right answer,
// so `add` asserts on one and the drain keeps the last valid row and says so on
// stderr, one line per refused intent. An intent naming an entity with no light
// is dropped silently: a destroyed entity is what a queue costs.
//
// It sits at "Rendering / Point light" in Add component, registered with the
// type (0221).
#pragma once

#include <ecs/world.h>
#include <scene/point_light_component.h>

#include <stdint.h>

// Registers the table (default row white, intensity 1, range 5 m, falloff 1,
// cast_shadows false, bounces 0, bounce strength 1; the transform it needs; its
// menu path; the whole-row intent as its replace)
// with room for `capacity`. Once per world, after the transform table (it
// asserts on none).
void voe_scene_point_light_register(voe_ecs_world *world, uint32_t capacity);

// Gives the entity its light, as given; a light the drain would refuse asserts.
// False when the table is full or the entity is not alive.
[[nodiscard]] bool voe_scene_point_light_add(voe_ecs_world *world,
					     voe_ecs_entity entity,
					     voe_scene_point_light light);

// Make this entity's point light the one the submitter says.
typedef struct {
	voe_ecs_entity entity;
	voe_scene_point_light light;
} voe_scene_point_light_intent;

// False when the queue is full — the system has not run for long enough.
[[nodiscard]] bool
voe_scene_point_light_submit(voe_ecs_world *world,
			     voe_scene_point_light_intent intent);

// Applies the replaces in submission order, and nothing else.
void voe_scene_point_light_system_run(voe_ecs_world *world);
