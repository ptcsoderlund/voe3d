// Where a thing is: a position, a rotation and a scale, and the matrix those
// three become. Read by anyone, const; written only through
// scene/transform_system.h.
//
// THERE IS NO PARENT AND NO HIERARCHY, DELIBERATELY. A transform is a world
// transform and nothing else, so the matrix below needs no walk and no cache and
// cannot be stale. A glTF node tree is flattened when it is imported — the
// importer composes the tree once and writes world transforms — and a `parent`
// component is a later card with a number attached, not something this shape is
// waiting for.
//
// THE MATRIX IS COMPUTED ON DEMAND AND NOT STORED. It is nine flops and a
// quaternion-to-matrix, it is wanted once per drawn object per frame, and a
// stored one is a second copy of the truth that something has to remember to
// invalidate. If that ever measures slow it becomes a cached column in this
// table, which is a change to this module and nothing else.
//
// TRANSLATE, THEN ROTATE, THEN SCALE — READ RIGHT TO LEFT. The matrix is
// T · R · S, so a point is scaled first, then rotated, then moved: the order
// anybody authoring one expects, and the order that makes scale a property of
// the object rather than of the world it sits in. Vectors are columns and
// composition reads right to left (CLAUDE.md), so the innermost operation is the
// rightmost factor.
#pragma once

#include <ecs/component.h>
#include <ecs/world.h>

#include <math/float3.h>
#include <math/float4x4.h>
#include <math/quat.h>

#include <stdint.h>

// A scale of one, a rotation of nothing and a position of nowhere is a zeroed
// struct with the scale filled in — there is no identity constant, because
// every caller so far has all three numbers to say.
typedef struct {
	voe_math_float3 position;
	voe_math_quat rotation;
	voe_math_float3 scale;
} voe_scene_transform;

// The key this component is registered against. Its address is its identity.
extern const struct voe_ecs_key voe_scene_transform_key;

// T · R · S, in this engine's row-major layout, ready to be handed to the GPU as
// sixteen floats.
voe_math_float4x4 voe_scene_transform_matrix(voe_scene_transform transform);

// NULL when the entity has no transform, or is not alive any more. The pointer
// is into the table and is good until the next add or remove.
const voe_scene_transform *voe_scene_transform_get(const voe_ecs_world *world,
						   voe_ecs_entity entity);

// The table, for a system that reads every one of them. rows[i] belongs to
// entities[i], and both are `count` long.
uint32_t voe_scene_transform_count(const voe_ecs_world *world);
const voe_scene_transform *voe_scene_transform_rows(const voe_ecs_world *world);
const voe_ecs_entity *voe_scene_transform_entities(const voe_ecs_world *world);
