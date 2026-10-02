// The sun: what colour it is, how much of it there is, and the fill that lifts
// what it leaves in shadow. Read by anyone, const; written only through
// scene/light_system.h. Which way it shines is not here: it is the transform's.
//
// IT IS DIRECTIONAL AND THEREFORE HAS NO POSITION, which is the whole of what
// makes it a sun rather than a lamp. Every surface is lit from the same
// direction with the same strength. Point and spot lights are a different
// component, not a field added here.
//
// THE DIRECTION IS THE TRANSFORM'S ROTATION (0222, 0271, 0273): the light
// travels along the rotation's -Z, the way the camera looks and the way glTF
// points a directional light. A transform is the one thing that turns anything
// in 3D space, so the rotate gizmo, undo and a parent turn the sun with no case
// of their own. A light with no transform shines along -Z. The direction is
// where the light goes, not where the sun is: a sun overhead shines (0, -1, 0).
// voe_scene_light_direction and voe_scene_light_facing convert between the two
// for a reader that wants a vector and a writer that holds one.
//
// THE FILL LIGHTS ONLY WHAT THE SUN DOES NOT REACH (0275, 0276): fill_colour
// times fill_intensity lifts shadowed sides and sides facing away, and fades
// out as the sun reaches the surface, so a surface in full sun looks the same
// whatever the fill. 0 is no fill, and a scene with none draws as it did
// before the fill existed.
//
// BOUNCES IS HOW MANY TIMES THIS LIGHT'S OWN LIGHT BOUNCES BACK OFF WHAT IT LIT
// (0317, 0319): 0 is none and costs nothing (0316), 1 is the bounce of 046, and
// 051 raises VOE_SCENE_LIGHT_BOUNCES_MAX. It is per light, not per scene. Its
// values are named, so a tool offers exactly the allowed counts and the scene
// text still holds the number; a count past the maximum is a bad light.
//
// INTENSITY IS A MULTIPLIER AND NOT A PHYSICAL UNIT. There is no exposure or
// tone mapping yet, so a number here is whatever looks right; 1 with white is
// what an unlit-looking scene turns into. Neither intensity may be negative.
//
// THE COLOURS ARE LINEAR, each channel 0 to 1, like every colour that reaches
// the GPU. They are multiplied into what the light does, so white changes
// nothing. sRGB lives at the two ends of the pipeline and never in a factor.
//
// THE STRUCT IS WRITTEN AS THE LIST OF ITS FIELDS (base/describe.h), so a build
// that asks for descriptions also has voe_scene_light_description(), and one
// that does not has the same struct and nothing more. Every field is authored
// and none is read-only.
#pragma once

#include <base/describe.h>
#include <base/imported.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <math/float3.h>
#include <math/quat.h>

#include <stdint.h>

// The colours are linear multipliers, (1, 1, 1) white; the intensities are
// not negative. The fill lifts only the shade, and bounces is at most
// VOE_SCENE_LIGHT_BOUNCES_MAX — see the header.
#define VOE_SCENE_LIGHT_FIELDS(F, F_READ_ONLY)      \
	F(voe_math_float3, colour, COLOUR)          \
	F(float, intensity, FLOAT32)                \
	F(voe_math_float3, fill_colour, COLOUR)     \
	F(float, fill_intensity, FLOAT32)           \
	F(uint32_t, bounces, UINT32)

// The most bounces a light may ask for.
#define VOE_SCENE_LIGHT_BOUNCES_MAX 1u

// The name of each allowed count, indexed by the count itself.
extern VOE_BASE_IMPORTED const char
	*const voe_scene_light_bounces_names[VOE_SCENE_LIGHT_BOUNCES_MAX + 1];

#define VOE_SCENE_LIGHT_NAMES(N) N(bounces, voe_scene_light_bounces_names)

VOE_BASE_DESCRIBE_STRUCT_NAMED(voe_scene_light, VOE_SCENE_LIGHT_FIELDS,
			       VOE_SCENE_LIGHT_NAMES)

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

// The unit direction a light turned by `rotation` travels: the rotation's -Z.
// The rotation need not be unit but must not be nought, which asserts.
voe_math_float3 voe_scene_light_direction(voe_math_quat rotation);

// The shortest-arc rotation whose -Z is `direction`, for code that holds a
// direction. Any length but nought, which asserts; along +Z, where every arc is
// as short, it is a half turn about Y.
voe_math_quat voe_scene_light_facing(voe_math_float3 direction);
