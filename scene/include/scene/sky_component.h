// The sky: how it looks, and nothing about light (0406). Read by anyone,
// const; written only through scene/sky_system.h.
//
// HAVING THE ROW IS THE SKY ON. A world with no sky row draws the old picture,
// byte for byte, so every scene saved before the sky existed keeps its look.
//
// THE ROW HOLDS NO LIGHT. The sky's light on surfaces is the world's first
// directional light's fill (scene/light_component.h), and the sky's sun is
// that light's direction and colour. The sky is as bright as authored; only
// the sun's disc and glow may be brighter.
//
// THE FIRST ROW IN TABLE ORDER IS THE ONE DRAWN. A world holds at most one by
// the caller's capacity; a second row, if a caller allows one, is ignored.
//
// THE FIELDS (0406 point 1), every number finite:
//   zenith, horizon, ground — linear colours, drawn as authored, as base
//                    colours are. Each channel at least 0.
//   horizon_height — 0 to 1: how far up from the horizon towards the zenith
//                    the horizon colour reaches; 0 a hard band.
//   sun_size       — the disc's angular radius in degrees, 0 (no disc) to
//                    VOE_SCENE_SKY_SUN_SIZE_MAX.
//   sun_glow       — 0 to 1: how strong and wide the halo round the sun is.
//   sunset         — 0 to 1: how much the sky turns to evening as the sun
//                    lowers; 0 keeps the authored colours at every sun angle.
//   haze           — metres at which half of a surface's colour is haze; 0 is
//                    no haze. At least 0.
//
// THE STRUCT IS WRITTEN AS THE LIST OF ITS FIELDS (base/describe.h), so a
// build that asks for descriptions also has voe_scene_sky_description(), and
// one that does not has the same struct and nothing more. Every field is
// authored, not read-only.
#pragma once

#include <base/describe.h>
#include <base/imported.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <math/float3.h>

#include <stdint.h>

// The widest sun disc, in degrees of angular radius.
#define VOE_SCENE_SKY_SUN_SIZE_MAX 10.0f

// Colours not negative, horizon_height, sun_glow and sunset 0 to 1, sun_size
// 0 to VOE_SCENE_SKY_SUN_SIZE_MAX, haze not negative — see the header.
#define VOE_SCENE_SKY_FIELDS(F, F_READ_ONLY) \
	F(voe_math_float3, zenith, COLOUR)   \
	F(voe_math_float3, horizon, COLOUR)  \
	F(voe_math_float3, ground, COLOUR)   \
	F(float, horizon_height, FLOAT32)    \
	F(float, sun_size, FLOAT32)          \
	F(float, sun_glow, FLOAT32)          \
	F(float, sunset, FLOAT32)            \
	F(float, haze, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(voe_scene_sky, VOE_SCENE_SKY_FIELDS)

// The key this component is registered against. Its address is its identity.
extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_scene_sky_key;

// NULL when the entity has no sky, or is not alive any more. The pointer is
// into the table and is good until the next add or remove.
const voe_scene_sky *voe_scene_sky_get(const voe_ecs_world *world,
				       voe_ecs_entity entity);

// The table, for the system that hands the sky to the GPU. rows[i] belongs to
// entities[i], and both are `count` long; rows[0] is the sky drawn.
uint32_t voe_scene_sky_count(const voe_ecs_world *world);
const voe_scene_sky *voe_scene_sky_rows(const voe_ecs_world *world);
const voe_ecs_entity *voe_scene_sky_entities(const voe_ecs_world *world);
