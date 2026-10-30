// The picture: how the whole drawn image is exposed and toned (0406 point 4).
// Read by anyone, const; written only through scene/picture_system.h.
//
// THESE ARE THE WHOLE PICTURE'S SETTINGS, NOT A CAMERA'S: the editor's views
// have no camera row, and they are exposed by this row as the game is.
//
// NO ROW ON IS x1 AND CLIP, THE OLD PICTURE, so a world without one on, a sky
// included, is drawn as it was before the row existed.
//
// THE FIRST ROW IN TABLE ORDER WHOSE ENTITY IS ON IS USED (0440); an off row
// is as no row (0427). A later row on, if a caller allows one, is ignored.
//
// THE FIELDS, every number finite:
//   exposure — stops, -VOE_SCENE_PICTURE_EXPOSURE_MAX to
//              VOE_SCENE_PICTURE_EXPOSURE_MAX: the colour is scaled by 2 to
//              this power, then the curve is applied.
//   tone     — the curve: Clip leaves the colour as it is, clipped at 1; Soft
//              is Khronos PBR Neutral. A UINT32 named by
//              voe_scene_picture_tone_names (0198), so a scene holds it as its
//              number.
// Overlays and the interface are untouched by both.
//
// EXPOSURE CARRIES ITS RANGE, IN STOPS, FOR A TOOL, which draws it as a slider
// (0498 point 5).
//
// BLOOM AND COLOUR GRADING WOULD JOIN THIS ROW LATER, as fields here.
//
// THE STRUCT IS WRITTEN AS THE LIST OF ITS FIELDS (base/describe.h), so a
// build that asks for descriptions also has voe_scene_picture_description(),
// and one that does not has the same struct and nothing more. Both fields are
// authored, not read-only.
#pragma once

#include <base/describe.h>
#include <base/imported.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <stdint.h>

// The tone values. A future value is a new constant here, never a reuse of
// one of these.
#define VOE_SCENE_PICTURE_TONE_CLIP 0u
#define VOE_SCENE_PICTURE_TONE_SOFT 1u

// The furthest exposure either way, in stops.
#define VOE_SCENE_PICTURE_EXPOSURE_MAX 16.0f

// The name of each tone value, indexed by the value's own number.
extern VOE_BASE_IMPORTED const char
	*const voe_scene_picture_tone_names[VOE_SCENE_PICTURE_TONE_SOFT + 1];

// exposure: stops within VOE_SCENE_PICTURE_EXPOSURE_MAX either way. tone: at
// most VOE_SCENE_PICTURE_TONE_SOFT — see the header.
#define VOE_SCENE_PICTURE_FIELDS(F, F_READ_ONLY) \
	F(float, exposure, FLOAT32)              \
	F(uint32_t, tone, UINT32)

#define VOE_SCENE_PICTURE_NAMES(N) N(tone, voe_scene_picture_tone_names)

#define VOE_SCENE_PICTURE_RANGES(R)                       \
	R(exposure, -VOE_SCENE_PICTURE_EXPOSURE_MAX,       \
	  VOE_SCENE_PICTURE_EXPOSURE_MAX, "stops")

VOE_BASE_DESCRIBE_STRUCT_NAMED_RANGED(voe_scene_picture,
				      VOE_SCENE_PICTURE_FIELDS,
				      VOE_SCENE_PICTURE_NAMES,
				      VOE_SCENE_PICTURE_RANGES)

// The key this component is registered against. Its address is its identity.
extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_scene_picture_key;

// NULL when the entity has no picture, or is not alive any more. The pointer
// is into the table and is good until the next add or remove.
const voe_scene_picture *voe_scene_picture_get(const voe_ecs_world *world,
					       voe_ecs_entity entity);

// The table, for the system that hands the picture to the GPU. rows[i]
// belongs to entities[i], and both are `count` long. The one used is the first
// row whose entity voe_scene_identity_off says is on (0440); an off row is as
// no row.
uint32_t voe_scene_picture_count(const voe_ecs_world *world);
const voe_scene_picture *voe_scene_picture_rows(const voe_ecs_world *world);
const voe_ecs_entity *voe_scene_picture_entities(const voe_ecs_world *world);
