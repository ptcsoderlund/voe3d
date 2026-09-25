// The sun: which way its light travels, what colour it is and how much of it
// there is. Read by anyone, const; written only through scene/light_system.h.
//
// IT IS DIRECTIONAL AND THEREFORE HAS NO POSITION, which is the whole of what
// makes it a sun rather than a lamp. Every surface in the world is lit from the
// same direction with the same strength, so there is nothing to fall off and
// nothing to be inside of. Point and spot lights are a later card and they are
// a different component, not a field added here.
//
// `direction` IS WHERE THE LIGHT GOES, NOT WHERE THE SUN IS, and the two are
// opposite. A sun overhead has a direction of (0, -1, 0): the light travels
// downwards. Getting this backwards lights exactly the faces that should be
// dark, which reads as a broken normal rather than a flipped light — so it is
// written here and again in render's shader, because those are the two ends of
// the same number.
//
// A READER ALWAYS GETS A UNIT DIRECTION. The system normalizes on the way in,
// both when a light is created and when an intent lands, so nothing downstream
// has to and no two readers can disagree about whether it has been done. A
// direction of nothing is the caller's bug and asserts.
//
// INTENSITY IS A MULTIPLIER AND NOT A PHYSICAL UNIT. There is no exposure, no
// tone mapping and no photometry in this engine yet, so a number here is
// whatever looks right on the screen in front of you; 1 with a white colour is
// what an unlit-looking scene turns into. The card that tone maps is the card
// that gives this a unit.
//
// THE COLOUR IS LINEAR, LIKE EVERY COLOUR THAT REACHES THE GPU. It is
// multiplied into what the light does, so a colour of (1, 1, 1) changes nothing
// and a warm sun is a colour with more red in it than blue. sRGB encoding lives
// at the two ends of the pipeline — a picture on the way in and the window on
// the way out — and never in a factor.
//
// THE STRUCT IS WRITTEN AS THE LIST OF ITS FIELDS (base/describe.h), so a build
// that asks for descriptions also has voe_scene_light_description(), and one that
// does not has the same struct and nothing more. Every field is authored and none
// is read-only.
#pragma once

#include <base/describe.h>
#include <base/imported.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <math/float3.h>

#include <stdint.h>

// direction is unit length, and the direction the light travels — see the
// header. colour is linear, and a multiplier: (1, 1, 1) is white.
#define VOE_SCENE_LIGHT_FIELDS(F, F_READ_ONLY) \
	F(voe_math_float3, direction, FLOAT3)  \
	F(voe_math_float3, colour, FLOAT3)     \
	F(float, intensity, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(voe_scene_light, VOE_SCENE_LIGHT_FIELDS)

// The key this component is registered against. Its address is its identity.
extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_scene_light_key;

// NULL when the entity has no light, or is not alive any more. The pointer is
// into the table and is good until the next add or remove.
const voe_scene_light *voe_scene_light_get(const voe_ecs_world *world,
					   voe_ecs_entity entity);

// The table, for the system that hands the light to the GPU. rows[i] belongs to
// entities[i], and both are `count` long.
uint32_t voe_scene_light_count(const voe_ecs_world *world);
const voe_scene_light *voe_scene_light_rows(const voe_ecs_world *world);
const voe_ecs_entity *voe_scene_light_entities(const voe_ecs_world *world);
