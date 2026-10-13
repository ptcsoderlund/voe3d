// The haze: how far surfaces fade into the air (0414, 0416, 0417). Read by
// anyone, const; written only through scene/haze_system.h.
//
// HAVING THE ROW IS HAZE ON. No row, no haze. It works with or without a sky
// row; neither row needs the other.
//
// THE FIRST ROW IN TABLE ORDER IS THE ONE USED. A world holds at most one by
// the caller's capacity; a second row, if a caller allows one, is ignored.
//
// THE FIELDS, `haze` and `start` finite and at least 0, `height` any finite:
//   haze   — metres past `start` at which half of a surface's colour is haze;
//            0 is no haze.
//   start  — metres within which no surface takes haze (0414); 0 is the fade
//            from the eye.
//   height — the world height, metres, y up, the haze layer reaches (0418).
//            It veils the sky, or the backdrop when there is no sky row, seen
//            through the layer, and nothing else: surfaces still fade by
//            distance only.
//
// THE COLOUR IT FADES TO IS NOT A FIELD (0416 points 2 to 4). Its hue is the
// sky's horizon hue when there is a sky row and the directional lights' colour
// when there is none; its brightness is always the directional lights'. So a
// dark sky over a bright scene keeps bright haze.
//
// THE STRUCT IS WRITTEN AS THE LIST OF ITS FIELDS (base/describe.h), so a
// build that asks for descriptions also has voe_scene_haze_description(), and
// one that does not has the same struct and nothing more. Every field is
// authored, not read-only.
#pragma once

#include <base/describe.h>
#include <base/imported.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <stdint.h>

// haze and start: finite and not negative; height: finite — see the header.
#define VOE_SCENE_HAZE_FIELDS(F, F_READ_ONLY) \
	F(float, haze, FLOAT32)               \
	F(float, start, FLOAT32)              \
	F(float, height, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(voe_scene_haze, VOE_SCENE_HAZE_FIELDS)

// The key this component is registered against. Its address is its identity.
extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_scene_haze_key;

// NULL when the entity has no haze, or is not alive any more. The pointer is
// into the table and is good until the next add or remove.
const voe_scene_haze *voe_scene_haze_get(const voe_ecs_world *world,
					 voe_ecs_entity entity);

// The table, for the system that hands the haze to the GPU. rows[i] belongs to
// entities[i], and both are `count` long; rows[0] is the one used.
uint32_t voe_scene_haze_count(const voe_ecs_world *world);
const voe_scene_haze *voe_scene_haze_rows(const voe_ecs_world *world);
const voe_ecs_entity *voe_scene_haze_entities(const voe_ecs_world *world);
