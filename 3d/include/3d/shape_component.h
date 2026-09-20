// Which built-in shape an entity draws, and in what colour, until its mesh and
// material are built for it. Read by anyone, const; written only through
// 3d/shape_system.h.
//
// A SHAPE IS HOW A CUBE SURVIVES A SAVE (ADR-0163). voe_3d_mesh and
// voe_3d_material hold GPU ids and are runtime-only (3d/mesh_component.h,
// 3d/material_component.h) — meaningless in a file and rebuilt at load. A shape
// holds none of that: one number, described, that a saved scene keeps and a
// shape system turns back into a mesh and a material on the next run.
//
// KIND NAMES ONE OF A SMALL, FIXED SET OF BUILT-IN SHAPES, not an arbitrary
// asset: a cube, a capsule and a cylinder (ADR-0191). A kind this build does
// not know draws nothing and is a warning (3d/shape_system.h) rather than a
// refusal, because an old scene opened by a newer or older build should still
// open. A path to a model is a different question — D-259 — and is not what
// this answers.
//
// KIND IS CHOSEN FROM ITS THREE NAMES (ADR-0195). A tool shows it as a dropdown
// of voe_3d_shape_kind_names, and a chosen kind reaches the row through the
// shape's intent the way a colour does (3d/shape_system.h), so the entity draws
// that kind from the next run on. The names are indexed by the kind's own number,
// which is why entry nought is NULL: nought is no kind, and a value a tool has no
// name for is shown as the number it is. A kind read off a file that this build
// does not know is still drawn as nothing with a warning rather than refused,
// because a file may be newer than the build.
//
// COLOUR IS EDITED, THROUGH THE SHAPE'S INTENT (3d/shape_system.h), and is
// linear RGB, each channel 0 to 1 (ADR-0191): the same numbers every shader
// reads, so the default grey is exactly the grey the shapes wore before a shape
// had a colour. It reaches the GPU in the drawn object's record, not in a
// material, because a material's record is written once and a colour changes
// while a person drags (3d/draw_system.h).
//
// THE DEFAULT ROW IS A GREY CUBE, and a shape needs a transform: without one
// the draw system has nowhere to put it (0193). voe_3d_shape_register finds the
// transform's type by its key, so the world must have registered transforms
// before it registers shapes.
//
// THE STRUCT IS WRITTEN AS THE LIST OF ITS FIELDS (base/describe.h), so a build
// that asks for descriptions also has voe_3d_shape_description(), and one that
// does not has the same struct and nothing more.
#pragma once

#include <base/describe.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <math/float3.h>

#include <stdint.h>

// The kinds defined so far. A future kind is a new constant here, never a
// reuse of one of these.
#define VOE_3D_SHAPE_CUBE 1u
#define VOE_3D_SHAPE_CAPSULE 2u
#define VOE_3D_SHAPE_CYLINDER 3u

// The colour a shape has until someone gives it another, linear.
#define VOE_3D_SHAPE_GREY ((voe_math_float3){ 0.7f, 0.7f, 0.7f })

// The name of each kind, indexed by the kind's own number; NULL for nought,
// which is no kind.
extern const char *const voe_3d_shape_kind_names[4];

#define VOE_3D_SHAPE_FIELDS(F, F_READ_ONLY) \
	F(uint32_t, kind, UINT32)           \
	F(voe_math_float3, colour, COLOUR)

#define VOE_3D_SHAPE_NAMES(N) N(kind, voe_3d_shape_kind_names)

VOE_BASE_DESCRIBE_STRUCT_NAMED(voe_3d_shape, VOE_3D_SHAPE_FIELDS,
			       VOE_3D_SHAPE_NAMES)

// The key this component is registered against. Its address is its identity.
extern const struct voe_ecs_key voe_3d_shape_key;

// Registers the table, its default row (a grey cube), that a shape needs a
// transform, and the shape's intent as its replace (3d/shape_system.h), with
// room for `capacity` intents. Once per world, after transforms are registered
// and before anything adds a shape.
void voe_3d_shape_register(voe_ecs_world *world, uint32_t capacity);

// Gives the entity its shape. False when the table is full or the entity is not
// alive. A direct call and not an intent: an intent changes a shape that
// exists, and this is what makes it exist.
[[nodiscard]] bool voe_3d_shape_add(voe_ecs_world *world, voe_ecs_entity entity,
				    voe_3d_shape shape);

// NULL when the entity has no shape or is not alive.
const voe_3d_shape *voe_3d_shape_get(const voe_ecs_world *world,
				     voe_ecs_entity entity);

// The table, for the shape system. rows[i] belongs to entities[i], and both are
// `count` long.
uint32_t voe_3d_shape_count(const voe_ecs_world *world);
const voe_3d_shape *voe_3d_shape_rows(const voe_ecs_world *world);
const voe_ecs_entity *voe_3d_shape_entities(const voe_ecs_world *world);
