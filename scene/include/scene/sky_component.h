// The sky: a backdrop, and nothing about light (0411, 0412, 0416). Read by
// anyone, const; written only through scene/sky_system.h.
//
// HAVING THE ROW IS THE SKY ON. A world with no sky row draws the old picture,
// byte for byte, so every scene saved before the sky existed keeps its look.
//
// THE ROW IS A BACKDROP AND LIGHTS NOTHING. No surface is lit by it. The
// lights draw their own discs and glows in it (scene/light_component.h). A
// dark sky under a bright light is a supported look. Game code animates the
// colours for a day and night cycle.
//
// THE HAZE IS NOT THIS ROW'S but the Haze row's (scene/haze_component.h). It
// takes this row's horizon hue but none of its brightness or strength (0416
// point 3).
//
// THE FIRST ROW IN TABLE ORDER IS THE ONE DRAWN. A world holds at most one by
// the caller's capacity; a second row, if a caller allows one, is ignored.
//
// THE FIELDS (0412 point 6), every number finite:
//   zenith, horizon, ground — linear colours, drawn as authored, as base
//                    colours are. Each channel at least 0.
//   horizon_height — 0 to 1: how far up from the horizon towards the zenith
//                    the horizon colour reaches; 0 a hard band.
//   strength       — scales the backdrop only, never a light's disc or glow
//                    or the haze. At least 0.
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

// Colours not negative, horizon_height 0 to 1, strength not negative — see the
// header.
#define VOE_SCENE_SKY_FIELDS(F, F_READ_ONLY) \
	F(voe_math_float3, zenith, COLOUR)   \
	F(voe_math_float3, horizon, COLOUR)  \
	F(voe_math_float3, ground, COLOUR)   \
	F(float, horizon_height, FLOAT32)    \
	F(float, strength, FLOAT32)

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
