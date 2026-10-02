// A point light: a lamp that shines from one place out to a range, and that can
// flash. Read by anyone, const; written only through scene/point_light_system.h.
//
// IT IS A LAMP BESIDE THE SUN, NOT A FIELD ON IT (0288, 0320): its own
// component, as many as a scene wants, where the sun (light_component.h) is one
// direction for every surface.
//
// IT SHINES FROM ITS TRANSFORM'S WORLD POSITION, and the transform's rotation and
// scale change nothing, so a parent carries a lamp as it carries anything
// (0281). RANGE IS WHERE IT ENDS, in metres: past it a surface gets none of it,
// and inside it the light falls off to nought at the range, never by the
// inverse square, so INTENSITY MEANS WHAT THE SUN'S DOES — a multiplier, 1 with
// white what an unlit-looking surface turns into. Neither it nor range is a
// physical unit, and range must be above nought.
//
// THE COLOUR IS LINEAR, each channel 0 to 1, multiplied into what the light
// does, like every colour that reaches the GPU.
//
// A FLASH (0320 point 2): `flash` is seconds, 0 a steady light. A light with a
// flash is dark until flashed, then fades linearly from full to dark over
// `flash` seconds. It is flashed when its glow row is first made if
// `flash_when_made` (a prefab lights an explosion with no code), on each flash
// intent naming it (game code lights a shot with one call), and on each
// accepted replace, so an edit in the Inspector shows. voe_scene_point_light_
// strength is what a reader draws with; the glow row is the system's.
//
// IT CASTS NO SHADOW AND BOUNCES NOTHING (0301): it has no option for either.
//
// THE STRUCT IS WRITTEN AS THE LIST OF ITS FIELDS (base/describe.h), as the
// sun's is. Every field is authored and none is read-only. The glow is a
// runtime-only row, never saved.
#pragma once

#include <base/describe.h>
#include <base/imported.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <math/float3.h>

#include <stdint.h>

// The colour a linear multiplier, (1, 1, 1) white; intensity and flash not
// negative, range above nought — see the header.
#define VOE_SCENE_POINT_LIGHT_FIELDS(F, F_READ_ONLY) \
	F(voe_math_float3, colour, COLOUR)           \
	F(float, intensity, FLOAT32)                 \
	F(float, range, FLOAT32)                     \
	F(float, flash, FLOAT32)                     \
	F(bool, flash_when_made, BOOL)

VOE_BASE_DESCRIBE_STRUCT(voe_scene_point_light, VOE_SCENE_POINT_LIGHT_FIELDS)

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

// The seconds of a flash still to fade, never below nought. One per light,
// made and counted down by the point light system.
typedef struct {
	float left;
} voe_scene_point_light_glow;

extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_scene_point_light_glow_key;

// The intensity to draw the light with now: its intensity when steady, its
// intensity times left / flash when it flashes, and 0 with no light, no glow
// row yet or none of the flash left.
float voe_scene_point_light_strength(const voe_ecs_world *world,
				     voe_ecs_entity entity);
