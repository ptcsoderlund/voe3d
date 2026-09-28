// Where a thing is: a position, a rotation and a scale, and the matrix those
// three become. Read by anyone, const; written only through
// scene/transform_system.h, which also says how often its drain reports.
//
// THE POSITION IS DOUBLE AND THE GPU NEVER SEES IT (ADR-0250). At 100 km a float
// keeps only ~8 mm; a difference of two doubles keeps it. So every matrix is
// about a point the caller names, normally the eye, and rotation and scale stay
// float.
//
// THERE IS NO PARENT AND NO HIERARCHY, DELIBERATELY. A transform is a world
// transform and nothing else, so the matrix below needs no walk and no cache and
// cannot be stale. A glTF node tree is flattened when it is imported — the
// importer composes the tree once and writes world transforms — and a `parent`
// component is a later card with a number attached, not something this shape is
// waiting for.
//
// THE ROTATION IS UNIT LENGTH AND EVERYTHING THAT READS ONE ASSUMES SO. The
// matrix below uses the short formula that only holds for a unit quaternion and
// asserts it (math/quat.h), so a row holding anything else is not a transform
// that draws wrong — it is one that stops the program. The drain is what keeps
// that true, whichever door an intent came through.
//
// WHAT THE DRAIN SETTLES, AND WHY EACH IS A CORRECTION OR A REFUSAL (ADR-0138 —
// correct where one nearest valid value exists, keep the last valid row where
// none does). It is the rotation that carries the rules, because it is the one
// field of the three whose valid values are a surface rather than every number a
// float can hold:
//
//   - A POSITION, ROTATION OR SCALE WITH A COMPONENT THAT IS NOT FINITE leaves
//     the row where it was. An infinity has no nearest float and a NaN is near
//     nothing, so there is no value to correct towards; and one NaN carried into
//     the matrix takes everything drawn through it off the screen rather than
//     showing up as one wrong object. Reported at `error:`.
//   - A ROTATION SHORTER THAN VOE_SCENE_TRANSFORM_LEAST_ROTATION also leaves the
//     row where it was. Normalising it would divide four numbers that are all
//     rounding error by a length that is all rounding error, and the direction
//     that came out would be the last bit's rather than the submitter's. So
//     does A ROTATION WHOSE COMPONENTS ARE FINITE AND WHOSE LENGTH IS NOT: four
//     squares are summed in float, so a large enough component overflows the
//     measurement rather than the value, and dividing by that infinity would
//     write four zeros. Both reported at `error:`.
//   - A ROTATION WHOSE LENGTH IS OFF 1 BY MORE THAN
//     VOE_SCENE_TRANSFORM_ROTATION_TOLERANCE is normalised and applied. There is
//     exactly one rotation pointing that way, so the correction is not a guess,
//     and the rest of the row is an edit worth keeping. Reported at `warning:`.
//   - A ROTATION INSIDE THE TOLERANCE is normalised and not reported. That is a
//     chain of composed rotations drifting in float, which is arithmetic rather
//     than anybody's mistake.
//
// SCALE HAS NO RULE BEYOND BEING FINITE, AND A ZERO ONE IS NOT A FAULT. Scaling
// a thing to nothing is a way of hiding it. The one place that cannot divide by
// it is the normal matrix, and 3d/src/normal_matrix.c answers it there rather
// than this module forbidding it here.
//
// THE STRUCT IS WRITTEN AS THE LIST OF ITS FIELDS (base/describe.h), so a build
// that asks for descriptions also has voe_scene_transform_description(), and one
// that does not has the same struct and nothing more.
#pragma once

#include <base/describe.h>
#include <base/imported.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <math/double3.h>
#include <math/float3.h>
#include <math/float4x4.h>
#include <math/quat.h>

#include <stdint.h>

// A scale of one, a rotation of nothing and a position of nowhere is a zeroed
// struct with the scale filled in — there is no identity constant, because
// every caller so far has all three numbers to say.
#define VOE_SCENE_TRANSFORM_FIELDS(F, F_READ_ONLY) \
	F(voe_math_double3, position, DOUBLE3)     \
	F(voe_math_quat, rotation, QUAT)           \
	F(voe_math_float3, scale, FLOAT3)

VOE_BASE_DESCRIBE_STRUCT(voe_scene_transform, VOE_SCENE_TRANSFORM_FIELDS)

// The shortest rotation the drain will normalise. Not zero, because it is the
// quotient that decides: at a length of 1e-6 the four components are divided by
// a number of their own order of magnitude, so the direction that comes out is
// the arithmetic's and not the submitter's.
#define VOE_SCENE_TRANSFORM_LEAST_ROTATION 1e-6f

// How far a rotation's length may sit from 1 before the drain says so. Wider
// than a composed chain of unit rotations drifts in float, and far narrower than
// anything a person dragging a field lands on, so what it separates is
// arithmetic from a mistake.
#define VOE_SCENE_TRANSFORM_ROTATION_TOLERANCE 1e-3f

// The key this component is registered against. Its address is its identity.
extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_scene_transform_key;

// T · R · S about `origin`, in this engine's row-major layout, ready to be handed
// to the GPU as sixteen floats. T is the position minus `origin`, subtracted in
// double and only then narrowed, so the matrix places the thing relative to a
// point the caller names, normally the eye.
//
// TRANSLATE, THEN ROTATE, THEN SCALE — READ RIGHT TO LEFT. The matrix is
// T · R · S, so a point is scaled first, then rotated, then moved: the order
// anybody authoring one expects, and the order that makes scale a property of
// the object rather than of the world it sits in. Vectors are columns and
// composition reads right to left (CLAUDE.md), so the innermost operation is the
// rightmost factor.
//
// THE MATRIX IS COMPUTED ON DEMAND AND NOT STORED. It is nine flops and a
// quaternion-to-matrix, it is wanted once per drawn object per frame, and a
// stored one is a second copy of the truth that something has to remember to
// invalidate. If that ever measures slow it becomes a cached column in this
// table, which is a change to this module and nothing else.
voe_math_float4x4 voe_scene_transform_matrix(voe_scene_transform transform,
					      voe_math_double3 origin);

// `child`'s world transform when `child` is relative to `parent` (ADR-0281):
// position = parent position + parent rotation · (parent scale ∘ child
// position), rotated in double so a far position keeps its millimetres
// (ADR-0250); rotation = parent · child, normalised; scale = parent scale ∘ child
// scale. The child is scaled, then turned, then moved by the parent — the same
// right-to-left order as the matrix above.
//
// EXACT WHILE SCALES ARE UNIFORM. A rotated child of a non-uniformly scaled
// parent shows no shear, because position, rotation and scale cannot hold one
// and every reader takes those three, not a matrix (ADR-0281).
voe_scene_transform voe_scene_transform_compose(voe_scene_transform parent,
						voe_scene_transform child);

// The inverse: the row that, composed under `parent`, gives `placed`. A zero
// component of the parent's scale gives zero for that component of the
// position and scale, never a division by zero or a NaN — nothing composed
// under a flattened axis can be told apart along it.
voe_scene_transform voe_scene_transform_relative(voe_scene_transform parent,
						 voe_scene_transform placed);

// NULL when the entity has no transform, or is not alive any more. The pointer
// is into the table and is good until the next add or remove.
const voe_scene_transform *voe_scene_transform_get(const voe_ecs_world *world,
						   voe_ecs_entity entity);

// The table, for a system that reads every one of them. rows[i] belongs to
// entities[i], and both are `count` long.
uint32_t voe_scene_transform_count(const voe_ecs_world *world);
const voe_scene_transform *voe_scene_transform_rows(const voe_ecs_world *world);
const voe_ecs_entity *voe_scene_transform_entities(const voe_ecs_world *world);
