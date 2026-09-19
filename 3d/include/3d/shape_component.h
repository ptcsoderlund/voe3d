// Which built-in shape an entity draws, until its mesh and material are built
// for it. Read by anyone, const; written only through 3d/shape_system.h.
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
// KIND IS READ-ONLY. It is set once, at creation, by the code that knows what
// it is making; nothing in this engine edits a shape's kind afterwards, so the
// inspector shows it and offers no way to change it, the same reason an
// identity's id is read-only (scene/identity_component.h).
//
// THE STRUCT IS WRITTEN AS THE LIST OF ITS FIELDS (base/describe.h), so a build
// that asks for descriptions also has voe_3d_shape_description(), and one that
// does not has the same struct and nothing more.
#pragma once

#include <base/describe.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <stdint.h>

// The kinds defined so far. A future kind is a new constant here, never a
// reuse of one of these.
#define VOE_3D_SHAPE_CUBE 1u
#define VOE_3D_SHAPE_CAPSULE 2u
#define VOE_3D_SHAPE_CYLINDER 3u

#define VOE_3D_SHAPE_FIELDS(F, F_READ_ONLY) \
	F_READ_ONLY(uint32_t, kind, UINT32)

VOE_BASE_DESCRIBE_STRUCT(voe_3d_shape, VOE_3D_SHAPE_FIELDS)

// The key this component is registered against. Its address is its identity.
extern const struct voe_ecs_key voe_3d_shape_key;

// Registers the table. Once per world, before anything adds a shape.
void voe_3d_shape_register(voe_ecs_world *world, uint32_t capacity);

// Gives the entity its shape. False when the table is full or the entity is not
// alive. A direct call and not an intent: kind is read-only, so there is nothing
// for an intent to change after creation — see the header.
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
