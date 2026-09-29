// The sweep (0293): a sphere moved from a double point along a float motion,
// answered with the first solid collider it touches.
//
//     voe_physics_obstacle obstacles[64];
//     uint32_t n = voe_physics_obstacles_gather(world, obstacles, 64);
//     voe_physics_hit hit;
//     if (voe_physics_sweep(obstacles, n, from, motion, 0.1f, self, &hit)) ...
//
// A RAY IS THE SWEEP WITH RADIUS 0; there is no second function.
//
// THE CALLER GATHERS, THEN SWEEPS MANY. The gather composes every collider's
// world shape once; each sweep reads that array. It is the caller's data,
// handed back on every call, not a cache beside the collider, so a query still
// only reads (0249 rule 1). A trigger is not gathered: a sweep passes through.
//
// THE ARITHMETIC IS FLOAT ABOUT `from` (0250): each obstacle's centre has
// `from` subtracted in double and only that offset becomes float, so a scene
// far from the origin hits as one beside it does.
//
// A SWEEP THAT STARTS OVERLAPPING hits at t 0, at `from`, with the normal
// against `motion`, or +Y when `motion` is nought.
//
// THE ONLY BROAD PHASE IS THE REACH: an obstacle whose centre is farther from
// the segment than its reach plus the radius is passed over before the exact
// test. Every obstacle is looked at; a grid would lift that ceiling.
//
// BOXES ARE GATHERED BUT NOT YET HIT: the sweep passes over them until the box
// test arrives (041 card 02).
#pragma once

#include <ecs/world.h>

#include <math/double3.h>
#include <math/float3.h>

#include <physics/shape.h>

#include <stdint.h>

typedef struct {
	voe_ecs_entity entity;
	voe_physics_shape shape;
	// The bounding radius about the shape's centre: a sphere's half.x, a
	// capsule's half.y, a box's length of half.
	float reach;
} voe_physics_obstacle;

typedef struct {
	voe_ecs_entity entity;
	// The fraction of `motion`, 0..1, at first touch.
	float t;
	// The point on the surface touched.
	voe_math_double3 point;
	// Unit; the surface's normal there, facing the sweep.
	voe_math_float3 normal;
} voe_physics_hit;

// Every collider that is not a trigger and has a shape, in table order,
// written to out. Returns how many were written; past `capacity` dropped.
uint32_t voe_physics_obstacles_gather(const voe_ecs_world *world,
				      voe_physics_obstacle *out,
				      uint32_t capacity);

// The first obstacle, but `ignore`, that a sphere of `radius` touches moving
// from `from` by `motion`, written to *out. False, *out left alone, when none.
[[nodiscard]] bool voe_physics_sweep(const voe_physics_obstacle *obstacles,
				     uint32_t count, voe_math_double3 from,
				     voe_math_float3 motion, float radius,
				     voe_ecs_entity ignore, voe_physics_hit *out);
