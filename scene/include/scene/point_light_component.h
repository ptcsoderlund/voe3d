// A point light: a lamp that shines from one place out to a range, fading on the
// way by its falloff. Read by anyone, const; written only through
// scene/point_light_system.h.
//
// IT IS A LAMP BESIDE THE SUN, NOT A FIELD ON IT (0288, 0320): its own
// component, as many as a scene wants, where the sun (light_component.h) is one
// direction for every surface, with neither range nor falloff.
//
// IT SHINES FROM ITS TRANSFORM'S WORLD POSITION, and the transform's rotation and
// scale change nothing, so a parent carries a lamp as it carries anything
// (0281). RANGE IS WHERE IT ENDS, in metres: past it a surface gets none of it,
// and inside it the light falls off to nought at the range, never by the
// inverse square, so INTENSITY MEANS WHAT THE SUN'S DOES — a multiplier, 1 with
// white what an unlit-looking surface turns into. Neither it nor range is a
// physical unit, and range must be above nought.
//
// FALLOFF IS HOW FAST IT FADES ON THE WAY TO ITS RANGE (0321 point 1, 0322
// point 1): d metres off it lights by saturate(1 - (d/range)^(2/falloff))^2.
// 1 is the default and 0320's look; low is an even pool with a soft rim, high a
// bright core. It lies within LEAST..MOST. A scene saved before it reads 1, and
// its two flash keys are ignored with a warning (0322 point 4).
//
// IT DOES NOT FLASH (0321 point 2): it shines at its intensity. Game code that
// wants a flash fades the intensity through the replace.
//
// THE COLOUR IS LINEAR, each channel 0 to 1, multiplied into what the light
// does, like every colour that reaches the GPU.
//
// IT CASTS NO SHADOW AND BOUNCES NOTHING (0301): it has no option for either.
//
// THE STRUCT IS WRITTEN AS THE LIST OF ITS FIELDS (base/describe.h), as the
// sun's is. Every field is authored and none is read-only.
#pragma once

#include <base/describe.h>
#include <base/imported.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <math/float3.h>

#include <stdint.h>

// The colour a linear multiplier, (1, 1, 1) white; intensity not negative,
// range above nought, falloff within LEAST..MOST — see the header.
#define VOE_SCENE_POINT_LIGHT_FIELDS(F, F_READ_ONLY) \
	F(voe_math_float3, colour, COLOUR)           \
	F(float, intensity, FLOAT32)                 \
	F(float, range, FLOAT32)                     \
	F(float, falloff, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(voe_scene_point_light, VOE_SCENE_POINT_LIGHT_FIELDS)

// The bounds a falloff lies within, both accepted (0322 point 2): exponents 8
// to 0.5. Past them the pool is a ring or a dot.
#define VOE_SCENE_POINT_LIGHT_FALLOFF_LEAST 0.25f
#define VOE_SCENE_POINT_LIGHT_FALLOFF_MOST 4.0f

// The key this component is registered against. Its address is its identity.
extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_scene_point_light_key;

// NULL when the entity has no point light, or is not alive any more. The
// pointer is into the table and is good until the next add or remove.
const voe_scene_point_light *
voe_scene_point_light_get(const voe_ecs_world *world, voe_ecs_entity entity);

// The table, for the code that hands the lights to the GPU. rows[i] belongs to
// entities[i], and both are `count` long.
uint32_t voe_scene_point_light_count(const voe_ecs_world *world);
const voe_scene_point_light *
voe_scene_point_light_rows(const voe_ecs_world *world);
const voe_ecs_entity *voe_scene_point_light_entities(const voe_ecs_world *world);
