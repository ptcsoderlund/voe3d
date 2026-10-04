// A light blocker's box in the world (0347): the scene row's size placed,
// turned and scaled by its entity's transform.
//
//     voe_physics_shape box;
//     if (voe_3d_light_blocker_shape(world, entity, frame.lag, &box)) { ... }
//
// WHAT THE BOX IS: kind VOE_PHYSICS_COLLIDER_BOX, its centre the transform's
// world place in double (ADR-0250), its rotation the world rotation, and its
// half |scale| × size / 2 along each local axis, so a negative scale is a
// mirrored box of the same size. A size of nought on an axis is a half of
// nought; who reads the box decides what that means.
//
// ONE ANSWER FOR THE PASS AND THE EDITOR. The pass's records
// (voe_3d_draw_system_light_blockers) and the editor's lines for the selected
// blocker both start here, so the lines drawn are the box that blocks.
//
// A PHYSICS SHAPE AND NOT A TYPE OF ITS OWN, because the collider's lines
// (voe_3d_collider_marker_quads) already draw one; the blocker collides with
// nothing all the same.
//
// Constraints: the world must have registered the light blocker table, as
// every lookup in `ecs` asserts on one it never registered.
#pragma once

#include <ecs/world.h>
#include <physics/shape.h>

// The box `entity`'s blocker is at `lag` of a step back (0254), into *out.
// False, *out untouched, when the entity is dead or has no blocker row or no
// transform.
[[nodiscard]] bool voe_3d_light_blocker_shape(const voe_ecs_world *world,
					      voe_ecs_entity entity, float lag,
					      voe_physics_shape *out);
