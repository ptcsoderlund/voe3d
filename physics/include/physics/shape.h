// A collider in the world: its kind, where its centre is, how it is turned and
// how big it is once its transform's scale is applied. What a query tests.
//
//     voe_physics_shape shape;
//     if (voe_physics_shape_of(world, entity, &shape)) { ... }
//
// HALF MEANS WHAT THE KIND SAYS. A box: half extents along its local axes. A
// sphere: its radius in `half.x`. A capsule: its radius in `half.x` and half its
// whole height, caps included, in `half.y`, never below the radius. The unused
// components are 0.
//
// SCALE IS TAKEN BY ITS SIZE, NOT ITS SIGN. A box's half is |scale| times half
// its size per axis. A sphere stays a sphere, so its radius uses the largest
// |scale|. A capsule stays upright on local Y: its radius uses the larger of
// |x| and |z|, its height |y|. A shape a scale would squash is thus the smallest
// round shape that holds it, never a smaller one.
//
// THE CENTRE IS DOUBLE AND EVERYTHING ELSE FLOAT (0250): a query subtracts
// centres in double and does its arithmetic in float about its own shape.
//
// THE SHAPE IS WORKED OUT ON EACH CALL AND NOT STORED, so a query only reads
// (0249 rule 1): no cache sits beside the collider to fall out of date.
#pragma once

#include <ecs/world.h>

#include <math/double3.h>
#include <math/float3.h>
#include <math/quat.h>

#include <stdint.h>

typedef struct {
	// One of VOE_PHYSICS_COLLIDER_BOX, _SPHERE or _CAPSULE.
	uint32_t kind;
	voe_math_double3 centre;
	voe_math_quat rotation;
	voe_math_float3 half;
} voe_physics_shape;

// The entity's collider in the world, written to *out. False, and *out left
// alone, when it has no collider, no transform or a kind this build does not
// know.
[[nodiscard]] bool voe_physics_shape_of(const voe_ecs_world *world,
					voe_ecs_entity entity,
					voe_physics_shape *out);
