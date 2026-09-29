// The sweep: a ray into a sphere; a swept sphere into a standing capsule's
// side and onto its cap; a miss beside; the nearer of two; `ignore`; a
// trigger left ungathered; a start inside; capacity honoured.
//
// EVERY SCENE RUNS TWICE, at the origin and moved by (1e6, 0, 1e6), and checks
// the same hit to 1e-4 m. A float there has a step of about 0.06, so only a
// sweep that subtracts centres in double first passes both (0250).
#include <base/arena.h>
#include <ecs/world.h>
#include <math/quat.h>
#include <physics/collider_component.h>
#include <physics/collider_system.h>
#include <physics/sweep.h>
#include <scene/parent_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#define ENTITIES 4
#define ROOM 8

static const voe_ecs_entity nobody = { 0 };

static voe_ecs_world *world_of(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = ENTITIES,
		.component_types = 3,
		.intent_types = 2,
		.structure_requests = 4,
		.structure_bytes = 128,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_transform_register(world, ENTITIES);
	voe_scene_parent_register(world, ENTITIES);
	voe_physics_collider_register(world, ENTITIES);
	return world;
}

// An upright entity with a collider of `kind` and `size` at `base` + `at`.
static voe_ecs_entity placed(voe_ecs_world *world, voe_math_double3 base,
			     voe_math_double3 at, uint32_t kind,
			     voe_math_float3 size, bool trigger)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){
			.position = { base.x + at.x, base.y + at.y,
				      base.z + at.z },
			.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_physics_collider_add(
		world, entity,
		(voe_physics_collider){
			.kind = kind, .size = size, .trigger = trigger }));
	return entity;
}

static voe_ecs_entity ball(voe_ecs_world *world, voe_math_double3 base,
			   double x, bool trigger)
{
	return placed(world, base, (voe_math_double3){ x, 0.0, 0.0 },
		      VOE_PHYSICS_COLLIDER_SPHERE,
		      (voe_math_float3){ 2.0f, 2.0f, 2.0f }, trigger);
}

static voe_math_double3 at(voe_math_double3 base, double x, double y, double z)
{
	return (voe_math_double3){ base.x + x, base.y + y, base.z + z };
}

// The hit's t, its point about `base` and its normal.
static void check_hit(voe_physics_hit hit, voe_math_double3 base, float t,
		      voe_math_float3 point, voe_math_float3 normal)
{
	VOE_TEST_CHECK_FLOAT(hit.t, t, 1e-5f);
	VOE_TEST_CHECK_FLOAT((float)(hit.point.x - base.x), point.x, 1e-4f);
	VOE_TEST_CHECK_FLOAT((float)(hit.point.y - base.y), point.y, 1e-4f);
	VOE_TEST_CHECK_FLOAT((float)(hit.point.z - base.z), point.z, 1e-4f);
	VOE_TEST_CHECK_FLOAT(hit.normal.x, normal.x, 1e-5f);
	VOE_TEST_CHECK_FLOAT(hit.normal.y, normal.y, 1e-5f);
	VOE_TEST_CHECK_FLOAT(hit.normal.z, normal.z, 1e-5f);
}

// A ray from x = -5 along +X into a sphere of radius 1 at the origin.
static void a_ray_hits_a_sphere(voe_base_arena *arena, voe_math_double3 base)
{
	voe_ecs_world *world = world_of(arena);
	voe_physics_obstacle obstacles[ROOM];
	voe_physics_hit hit;
	uint32_t n;

	(void)ball(world, base, 0.0, false);
	n = voe_physics_obstacles_gather(world, obstacles, ROOM);
	VOE_TEST_CHECK_INT(n, 1);
	VOE_TEST_CHECK(voe_physics_sweep(obstacles, n, at(base, -5, 0, 0),
					 (voe_math_float3){ 10, 0, 0 }, 0.0f,
					 nobody, &hit));
	check_hit(hit, base, 0.4f, (voe_math_float3){ -1, 0, 0 },
		  (voe_math_float3){ -1, 0, 0 });
}

// A capsule of radius 0.5 and height 4 standing at the origin: a sphere of
// radius 0.5 swept into its side, then down onto its cap; one passing beside.
static void a_sphere_against_a_capsule(voe_base_arena *arena,
				       voe_math_double3 base)
{
	voe_ecs_world *world = world_of(arena);
	voe_physics_obstacle obstacles[ROOM];
	voe_physics_hit hit;
	uint32_t n;

	(void)placed(world, base, (voe_math_double3){ 0, 0, 0 },
		     VOE_PHYSICS_COLLIDER_CAPSULE,
		     (voe_math_float3){ 1.0f, 4.0f, 1.0f }, false);
	n = voe_physics_obstacles_gather(world, obstacles, ROOM);

	VOE_TEST_CHECK(voe_physics_sweep(obstacles, n, at(base, -5, 0.5, 0),
					 (voe_math_float3){ 10, 0, 0 }, 0.5f,
					 nobody, &hit));
	check_hit(hit, base, 0.4f, (voe_math_float3){ -0.5f, 0.5f, 0 },
		  (voe_math_float3){ -1, 0, 0 });

	VOE_TEST_CHECK(voe_physics_sweep(obstacles, n, at(base, 0, 5, 0),
					 (voe_math_float3){ 0, -10, 0 }, 0.5f,
					 nobody, &hit));
	check_hit(hit, base, 0.25f, (voe_math_float3){ 0, 2, 0 },
		  (voe_math_float3){ 0, 1, 0 });

	VOE_TEST_CHECK(!voe_physics_sweep(obstacles, n, at(base, -5, 0, 3),
					  (voe_math_float3){ 10, 0, 0 }, 0.5f,
					  nobody, &hit));
}

// Spheres at x = 4 and x = 2, the farther first: the nearer is hit, and the
// farther once the nearer is ignored.
static void the_nearer_wins(voe_base_arena *arena, voe_math_double3 base)
{
	voe_ecs_world *world = world_of(arena);
	voe_physics_obstacle obstacles[ROOM];
	voe_physics_hit hit;
	const voe_ecs_entity far = ball(world, base, 4.0, false);
	const voe_ecs_entity near = ball(world, base, 2.0, false);
	const uint32_t n = voe_physics_obstacles_gather(world, obstacles, ROOM);

	VOE_TEST_CHECK(voe_physics_sweep(obstacles, n, base,
					 (voe_math_float3){ 10, 0, 0 }, 0.0f,
					 nobody, &hit));
	VOE_TEST_CHECK(hit.entity.index == near.index);
	check_hit(hit, base, 0.1f, (voe_math_float3){ 1, 0, 0 },
		  (voe_math_float3){ -1, 0, 0 });

	VOE_TEST_CHECK(voe_physics_sweep(obstacles, n, base,
					 (voe_math_float3){ 10, 0, 0 }, 0.0f,
					 near, &hit));
	VOE_TEST_CHECK(hit.entity.index == far.index);
	VOE_TEST_CHECK_FLOAT(hit.t, 0.3f, 1e-5f);
}

// A trigger beside a solid: one gathered; capacity 1 of two solids writes one.
static void triggers_and_capacity(voe_base_arena *arena,
				  voe_math_double3 base)
{
	voe_ecs_world *world = world_of(arena);
	voe_physics_obstacle obstacles[ROOM];
	const voe_ecs_entity solid = ball(world, base, 0.0, false);
	uint32_t n;

	(void)ball(world, base, 5.0, true);
	n = voe_physics_obstacles_gather(world, obstacles, ROOM);
	VOE_TEST_CHECK_INT(n, 1);
	if (n == 1)
		VOE_TEST_CHECK(obstacles[0].entity.index == solid.index);

	(void)ball(world, base, 10.0, false);
	VOE_TEST_CHECK_INT(voe_physics_obstacles_gather(world, obstacles, ROOM),
			   2);
	VOE_TEST_CHECK_INT(voe_physics_obstacles_gather(world, obstacles, 1), 1);
}

// A sphere of radius 0.5 starting inside one of radius 1, moving along +Z:
// a hit at t 0 with the normal against the motion.
static void a_start_inside(voe_base_arena *arena, voe_math_double3 base)
{
	voe_ecs_world *world = world_of(arena);
	voe_physics_obstacle obstacles[ROOM];
	voe_physics_hit hit;
	uint32_t n;

	(void)ball(world, base, 0.0, false);
	n = voe_physics_obstacles_gather(world, obstacles, ROOM);
	VOE_TEST_CHECK(voe_physics_sweep(obstacles, n, at(base, 0.5, 0, 0),
					 (voe_math_float3){ 0, 0, 2 }, 0.5f,
					 nobody, &hit));
	check_hit(hit, base, 0.0f, (voe_math_float3){ 0.5f, 0, 0.5f },
		  (voe_math_float3){ 0, 0, -1 });
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);
	const voe_math_double3 places[2] = { { 0.0, 0.0, 0.0 },
					     { 1000000.0, 0.0, 1000000.0 } };

	for (int i = 0; i < 2; i++) {
		a_ray_hits_a_sphere(arena, places[i]);
		a_sphere_against_a_capsule(arena, places[i]);
		the_nearer_wins(arena, places[i]);
		triggers_and_capacity(arena, places[i]);
		a_start_inside(arena, places[i]);
	}

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
