// The sky: how strongly it lights, how thick its haze is, and how the picture
// is exposed and toned (0405). Read by anyone, const; written only through
// scene/sky_system.h.
//
// HAVING THE ROW IS THE SKY ON. A world with no sky row draws the old picture,
// byte for byte, so every scene saved before the sky existed keeps its look.
//
// THE SUN IT FOLLOWS IS THE WORLD'S FIRST DIRECTIONAL LIGHT (scene/light_component.h):
// its direction places the sky's sun and its colour times intensity lights the
// air. This row holds no direction and no colour of its own.
//
// THE FIRST ROW IN TABLE ORDER IS THE ONE DRAWN. A world holds at most one by
// the caller's capacity; a second row, if a caller allows one, is ignored.
//
// THE FIELDS (0405 point 1):
//   light    — how strongly the sky lights lit surfaces, a multiplier; 1 is
//              the honest sky, 0 none. Finite, at least 0.
//   haze     — metres at which half of a surface's colour is haze; 0 is no
//              haze. Finite, at least 0.
//   exposure — stops the picture is brightened (× 2^exposure); 0 is as is.
//              Finite, from -VOE_SCENE_SKY_EXPOSURE_MAX to +MAX.
//   tone     — a UINT32 named by voe_scene_sky_tone_names: Clip (as is, the
//              target clamps) or Soft (Khronos PBR Neutral). At most Soft.
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

#include <stdint.h>

// The Tone values. A future value is a new constant here, never a reuse.
#define VOE_SCENE_SKY_TONE_CLIP 0u
#define VOE_SCENE_SKY_TONE_SOFT 1u

// The most stops exposure may go either way.
#define VOE_SCENE_SKY_EXPOSURE_MAX 16.0f

// The name of each Tone value, indexed by the value's own number.
extern VOE_BASE_IMPORTED const char *const voe_scene_sky_tone_names[2];

// light and haze not negative, exposure within ±VOE_SCENE_SKY_EXPOSURE_MAX,
// tone at most Soft, all finite — see the header.
#define VOE_SCENE_SKY_FIELDS(F, F_READ_ONLY) \
	F(float, light, FLOAT32)             \
	F(float, haze, FLOAT32)              \
	F(float, exposure, FLOAT32)          \
	F(uint32_t, tone, UINT32)

#define VOE_SCENE_SKY_NAMES(N) N(tone, voe_scene_sky_tone_names)

VOE_BASE_DESCRIBE_STRUCT_NAMED(voe_scene_sky, VOE_SCENE_SKY_FIELDS,
			       VOE_SCENE_SKY_NAMES)

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
