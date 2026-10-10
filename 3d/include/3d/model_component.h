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
// MATERIALS[I] IS WORN BY PART I in the store's bake order (3d/models.h), for
// the first VOE_3D_MODEL_MATERIALS parts (0399 point 6): a `.material` path as
// `path` is. Empty, or a path the store holds no loaded material for, keeps the
// file's own material; empty by default and in a file without them. A
// landscape row's ground ignores them: its layers are 086's.
//
// It sits at "Rendering / Model" in Add component (ecs/component.h, 0221), and
// its intent is its replace, the whole row, as the shape's is
// (3d/shape_system.h).
//
// THE DRAIN drops an intent naming a dead entity or one with no model,
// silently. A path, or one of the materials, with no NUL in its 128 bytes has its last byte made NUL and
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

// The parts a row can give a material of its own, the first in bake order.
#define VOE_3D_MODEL_MATERIALS 8

#define VOE_3D_MODEL_FIELDS(F, F_READ_ONLY)        \
	F(char, path, CHAR, VOE_3D_MODEL_PATH) \
	F(bool, cast_shadows, BOOL)            \
	F(float, fade, FLOAT32)                \
	F(char, materials, CHAR, VOE_3D_MODEL_MATERIALS, VOE_3D_MODEL_PATH)

VOE_BASE_DESCRIBE_STRUCT(voe_3d_model, VOE_3D_MODEL_FIELDS)

// The key this component is registered against. Its address is its identity.
extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_3d_model_key;

// Change this entity's model to the submitter's row.
typedef struct {
	voe_ecs_entity entity;
	voe_3d_model model;
} voe_3d_model_intent;

// Registers the table, its default row (the empty path, casting), that a model needs a
// transform, its menu path, and the intent as its replace with room for
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
