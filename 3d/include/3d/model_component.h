// Which model file an entity wears, as a path, and the intent and system that
// change it. Read by anyone, const; written through voe_3d_model_submit.
//
//     voe_scene_transform_register(world, capacity);   // first: a model needs one
//     voe_3d_model_register(world, capacity);
//     if (!voe_3d_model_submit(world, (voe_3d_model_intent){
//                 .entity = entity, .model = { .path = "Assets/hull.glb" } }))
//             ...                                       // the queue is full
//     voe_3d_model_system_run(world);                   // once a step
//
// THE PATH IS RELATIVE TO THE PROJECT FOLDER, WITH `/` SEPARATORS
// (Assets/hull.glb), NUL-terminated within its 128 bytes (0277 point 1). An
// empty path draws nothing, silently. Nothing here reads a file or draws: what
// loads and draws a model is 3d/models.h, and whoever reads files hands it the
// bytes (0277 points 2-4), so 3d stays off platform.
//
// A PATH, NOT AN ID, because the Inspector, the scene text, undo and the cook
// already carry CHAR fields: a model survives save, undo and ship with no new
// format, and one file loaded once serves every thing that names it.
//
// IT NEEDS A TRANSFORM (the draw has nowhere to put it without one), so the
// world registers transforms before models. The default row is the empty path,
// casting.
//
// CAST_SHADOWS FALSE LEAVES EVERY PART OF THE MODEL OUT OF EVERY SHADOW AND
// BOUNCE MAP while it is still drawn, lit and shadowed (0301, 0324). True by
// default and in a file without it; zero is false, so a row built from a
// literal names it.
//
// FADE 0 DRAWS AS EVER, 1 IS GONE, BETWEEN IS SEE-THROUGH BY THAT MUCH (0336).
// 0 by default, in a file without it and in a literal, so nothing written
// before it changes; game code fades a thing through the intent.
//
// BLEND WITH GROUND TAKES ON THE PAINTED GROUND NEAR WHERE THE MODEL MEETS IT,
// fading into its own material `blend_height` metres up (0481 points 1, 3).
// Off by default, in a file without it and in a literal. The height is 0.5 in
// the default row and a file without it; a literal's 0 blends nothing.
//
// BLEND HEIGHT AND FADE CARRY THEIR RANGES for a tool's slider (0494, 0498):
// fade 0-1, the height VOE_3D_MODEL_BLEND_HEIGHT_LEAST to _GREATEST metres, the
// range the draw clamps it to, so the slider and the draw agree. A foliage
// kind's Blend height is shown over this same range (0494 point 2, 0498 point 3).
//
// A PART'S MATERIAL IS THE MODEL'S, NEVER THE ROW'S (0404): the store gives
// each part the `.material` its model's parts file names (3d/models.h), so
// every placed copy of a model wears the same.
//
// It sits at "Rendering / Model" in Add component (ecs/component.h, 0221), and
// its intent is its replace, the whole row, as the shape's is
// (3d/shape_system.h).
//
// THE DRAIN drops an intent naming a dead entity or one with no model,
// silently. A path with no NUL in its 128 bytes has its last byte made NUL and
// is reported as corrected, edge-triggered as the shape's corrections are: the
// first of a run named on stderr, the rest counted, the count said once a run
// corrects nothing. The run and its count are file-scope statics, per process,
// the trade 3d/src/shape_system.c makes.
#pragma once

#include <base/describe.h>
#include <base/imported.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <stdint.h>

// The path's bytes, terminating NUL included.
#define VOE_3D_MODEL_PATH 128

#define VOE_3D_MODEL_FIELDS(F, F_READ_ONLY)        \
	F(char, path, CHAR, VOE_3D_MODEL_PATH) \
	F(bool, cast_shadows, BOOL)            \
	F(float, fade, FLOAT32)                \
	F(bool, blend_with_ground, BOOL)       \
	F(float, blend_height, FLOAT32)

// A blend height's ends, metres: what the draw clamps to and its range says.
#define VOE_3D_MODEL_BLEND_HEIGHT_LEAST 0.05f
#define VOE_3D_MODEL_BLEND_HEIGHT_GREATEST 10.0f

#define VOE_3D_MODEL_RANGES(R)                                        \
	R(fade, 0.0, 1.0, "")                                         \
	R(blend_height, VOE_3D_MODEL_BLEND_HEIGHT_LEAST,              \
	  VOE_3D_MODEL_BLEND_HEIGHT_GREATEST, "m")

VOE_BASE_DESCRIBE_STRUCT_RANGED(voe_3d_model, VOE_3D_MODEL_FIELDS,
				VOE_3D_MODEL_RANGES)

// The key this component is registered against. Its address is its identity.
extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_3d_model_key;

// Change this entity's model to the submitter's row.
typedef struct {
	voe_ecs_entity entity;
	voe_3d_model model;
} voe_3d_model_intent;

// Registers the table, its default row (the empty path, casting, not blending
// with ground, a blend height of 0.5 m), that a model needs a transform, its menu path, and the intent as its replace with room for
// `capacity` intents. Once per world, after transforms are registered.
void voe_3d_model_register(voe_ecs_world *world, uint32_t capacity);

// Gives the entity its model. False when the table is full or the entity is not
// alive. A direct call: an intent changes a model that exists.
[[nodiscard]] bool voe_3d_model_add(voe_ecs_world *world, voe_ecs_entity entity,
				    voe_3d_model model);

// NULL when the entity has no model or is not alive.
const voe_3d_model *voe_3d_model_get(const voe_ecs_world *world,
				     voe_ecs_entity entity);

// The table. rows[i] belongs to entities[i], and both are `count` long.
uint32_t voe_3d_model_count(const voe_ecs_world *world);
const voe_3d_model *voe_3d_model_rows(const voe_ecs_world *world);
const voe_ecs_entity *voe_3d_model_entities(const voe_ecs_world *world);

// False when the queue is full: the system has not run for long enough.
[[nodiscard]] bool voe_3d_model_submit(voe_ecs_world *world,
				       voe_3d_model_intent intent);

// Drains the model's intents in submission order and empties the queue; see the
// header for what is dropped and what is corrected.
void voe_3d_model_system_run(voe_ecs_world *world);
