// The one query (0253 point 3): what a sphere or capsule overlaps, as contacts
// that say which way and how far to push it out. It only reads (0249 rule 1).
//
//     voe_physics_contact contacts[8];
//     uint32_t n = voe_physics_overlap(world, shape, self, contacts, 8);
//
// THE ARITHMETIC IS FLOAT ABOUT THE QUERY'S CENTRE (0250). Each other shape's
// centre has the query's subtracted in double first, and only that offset is
// turned to float, so a scene far from the origin gives the same contacts as
// one beside it.
//
// A SPHERE IS A CAPSULE WITH NO SEGMENT. The query is a segment along its local
// Y plus a radius; a sphere's segment is one point. A box is not a query shape
// and asserts: nothing asks with one yet (rule 10).
//
// RAY AND SWEEP ARE NOT HERE YET because nothing calls them: the body moves in
// substeps and pushes out of what it overlaps (0253 point 4). They arrive with
// their first caller.
//
// EVERY COLLIDER IS TESTED, one after another, no broad phase. That is the
// ceiling at a few hundred colliders; a grid or tree beside the table, kept by
// the collider system rather than by the query, is what would lift it.
#pragma once

#include <ecs/world.h>

#include <math/float3.h>

#include <physics/shape.h>

#include <stdint.h>

typedef struct {
	voe_ecs_entity entity;
	// Unit; the way to push the query out of this collider.
	voe_math_float3 normal;
	// How far along `normal`, above 0.
	float depth;
	// The collider is a trigger: it blocks nothing, but it is found.
	bool trigger;
} voe_physics_contact;

// Every collider the query overlaps by more than nothing, except `ignore`'s,
// written to out. Returns how many were written; past `capacity` the rest are
// dropped. The query is a sphere or a capsule.
uint32_t voe_physics_overlap(const voe_ecs_world *world,
			     voe_physics_shape query, voe_ecs_entity ignore,
			     voe_physics_contact *out, uint32_t capacity);
