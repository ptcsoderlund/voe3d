// The sweep: a ray into a sphere; a swept sphere into a standing capsule's
// side and onto its cap; a miss beside; the nearer of two; `ignore`; a
// trigger left ungathered; a start inside; capacity honoured. Boxes: a ray
// into a turned face; a sphere past a corner missing where a sharp grown box
// would hit, and grazing an edge; a thin box not tunnelled; a box under a
// scaled, turned parent; a ray starting inside.
//
// EVERY SCENE RUNS TWICE, at the origin and moved by (1e6, 0, 1e6), and checks
// the same hit to 1e-4 m. A float there has a step of about 0.06, so only a
// sweep that subtracts centres in double first passes both (0250).
#include <base/arena.h>
#include <ecs/structure.h>
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

// An entity turned by `rotation` with a collider of `kind` and `size` at
// `base` + `at`.
static voe_ecs_entity placed_turned(voe_ecs_world *world,
				    voe_math_double3 base, voe_math_double3 at,
				    voe_math_quat rotation, uint32_t kind,
				    voe_math_float3 size, bool trigger)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){
			.position = { base.x + at.x, base.y + at.y,
				      base.z + at.z },
			.rotation = rotation,
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_physics_collider_add(
		world, entity,
		(voe_physics_collider){
			.kind = kind, .size = size, .trigger = trigger }));
	return entity;
}

// An upright entity with a collider of `kind` and `size` at `base` + `at`.
static voe_ecs_entity placed(voe_ecs_world *world, voe_math_double3 base,
			     voe_math_double3 at, uint32_t kind,
			     voe_math_float3 size, bool trigger)
{
	return placed_turned(world, base, at,
			     (voe_math_quat){ 0.0f, 0.0f, 0.0f, 1.0f }, kind,
			     size, trigger);
}

// A box of `size` standing unturned at `base`.
static voe_ecs_entity box(voe_ecs_world *world, voe_math_double3 base,
			  voe_math_float3 size)
{
	return placed(world, base, (voe_math_double3){ 0, 0, 0 },
		      VOE_PHYSICS_COLLIDER_BOX, size, false);
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

// A cube of 2 turned 45 degrees about Y: a ray along +X at z 0.5 meets its
// local -X face at x = 0.5 - sqrt 2, the normal half way between -X and +Z.
static void a_ray_hits_a_turned_box(voe_base_arena *arena,
				    voe_math_double3 base)
{
	voe_ecs_world *world = world_of(arena);
	voe_physics_obstacle obstacles[ROOM];
	voe_physics_hit hit;
	uint32_t n;

	(void)placed_turned(world, base, (voe_math_double3){ 0, 0, 0 },
			    voe_math_quat_from_axis_angle(
				    (voe_math_float3){ 0, 1, 0 },
				    0.78539816f),
			    VOE_PHYSICS_COLLIDER_BOX,
			    (voe_math_float3){ 2, 2, 2 }, false);
	n = voe_physics_obstacles_gather(world, obstacles, ROOM);
	VOE_TEST_CHECK(voe_physics_sweep(obstacles, n, at(base, -5, 0, 0.5),
					 (voe_math_float3){ 10, 0, 0 }, 0.0f,
					 nobody, &hit));
	check_hit(hit, base, 0.40857864f,
		  (voe_math_float3){ -0.91421356f, 0, 0.5f },
		  (voe_math_float3){ -0.70710678f, 0, 0.70710678f });
}

// A cube of 2: a sphere of 0.5 whose path passes 0.6 m out from the corner
// (1, 1, 1) along the diagonal misses, though it enters the box grown by 0.5.
// One at y 1.3 along +X grazes the edge at (-1, 1): its centre is 0.5 from
// the edge at x -1.4, and the normal points from the edge to it.
static void a_sphere_at_corners_and_edges(voe_base_arena *arena,
					  voe_math_double3 base)
{
	voe_ecs_world *world = world_of(arena);
	voe_physics_obstacle obstacles[ROOM];
	voe_physics_hit hit;
	const double past = 1.0 + 0.6 / 1.7320508075688772;
	uint32_t n;

	(void)box(world, base, (voe_math_float3){ 2, 2, 2 });
	n = voe_physics_obstacles_gather(world, obstacles, ROOM);
	VOE_TEST_CHECK(!voe_physics_sweep(
		obstacles, n, at(base, past - 5.0, past + 5.0, past),
		(voe_math_float3){ 10, -10, 0 }, 0.5f, nobody, &hit));

	VOE_TEST_CHECK(voe_physics_sweep(obstacles, n, at(base, -5, 1.3, 0),
					 (voe_math_float3){ 10, 0, 0 }, 0.5f,
					 nobody, &hit));
	check_hit(hit, base, 0.36f, (voe_math_float3){ -1, 1, 0 },
		  (voe_math_float3){ -0.8f, 0.6f, 0 });
}

// A box 0.05 m thick along X: a sphere of 0.05 swept 100 m through it in one
// sweep is stopped at its face.
static void a_thin_box_is_not_tunnelled(voe_base_arena *arena,
					voe_math_double3 base)
{
	voe_ecs_world *world = world_of(arena);
	voe_physics_obstacle obstacles[ROOM];
	voe_physics_hit hit;
	uint32_t n;

	(void)box(world, base, (voe_math_float3){ 0.05f, 2, 2 });
	n = voe_physics_obstacles_gather(world, obstacles, ROOM);
	VOE_TEST_CHECK(voe_physics_sweep(obstacles, n, at(base, -50, 0, 0),
					 (voe_math_float3){ 100, 0, 0 }, 0.05f,
					 nobody, &hit));
	check_hit(hit, base, 0.49925f, (voe_math_float3){ -0.025f, 0, 0 },
		  (voe_math_float3){ -1, 0, 0 });
}

// A cube of 1 whose row is (1, 0, 0) under a parent at (10, 0, 0), scaled 2
// and turned 90 degrees about Y: it stands at (10, 0, -2) with a half of 1.
// A ray down -Z there meets it at z -1; one at the row's place misses.
static void a_child_box_is_hit_at_its_world_place(voe_base_arena *arena,
						  voe_math_double3 base)
{
	voe_ecs_world *world = world_of(arena);
	voe_physics_obstacle obstacles[ROOM];
	voe_physics_hit hit;
	const voe_math_quat upright = { 0.0f, 0.0f, 0.0f, 1.0f };
	const voe_ecs_entity child = box(world, base, (voe_math_float3){ 1, 1, 1 });
	voe_ecs_entity parent = { 0 };
	uint32_t n;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &parent));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, parent,
		(voe_scene_transform){
			.position = { base.x + 10.0, base.y, base.z },
			.rotation = voe_math_quat_from_axis_angle(
				(voe_math_float3){ 0, 1, 0 }, 1.57079633f),
			.scale = { 2.0f, 2.0f, 2.0f } }));
	VOE_TEST_CHECK(voe_scene_parent_set(world, child, parent));
	voe_ecs_structure_apply(world);
	voe_scene_transform_system_run(world);
	VOE_TEST_CHECK(voe_scene_transform_submit(
		world, (voe_scene_transform_intent){
			       .entity = child,
			       .transform = { .position = { 1.0, 0.0, 0.0 },
					      .rotation = upright,
					      .scale = { 1.0f, 1.0f, 1.0f } } }));
	voe_scene_transform_system_run(world);

	n = voe_physics_obstacles_gather(world, obstacles, ROOM);
	VOE_TEST_CHECK(voe_physics_sweep(obstacles, n, at(base, 10, 0, 5),
					 (voe_math_float3){ 0, 0, -10 }, 0.0f,
					 nobody, &hit));
	VOE_TEST_CHECK(hit.entity.index == child.index);
	check_hit(hit, base, 0.6f, (voe_math_float3){ 10, 0, -1 },
		  (voe_math_float3){ 0, 0, 1 });
	VOE_TEST_CHECK(!voe_physics_sweep(obstacles, n, at(base, 1, 0, 5),
					  (voe_math_float3){ 0, 0, -10 }, 0.0f,
					  nobody, &hit));
}

// A ray starting inside a cube of 2, moving along +Z: a hit at t 0 at its
// start, the normal against the motion.
static void a_ray_starts_inside_a_box(voe_base_arena *arena,
				      voe_math_double3 base)
{
	voe_ecs_world *world = world_of(arena);
	voe_physics_obstacle obstacles[ROOM];
	voe_physics_hit hit;
	uint32_t n;

	(void)box(world, base, (voe_math_float3){ 2, 2, 2 });
	n = voe_physics_obstacles_gather(world, obstacles, ROOM);
	VOE_TEST_CHECK(voe_physics_sweep(obstacles, n, at(base, 0.5, 0, 0),
					 (voe_math_float3){ 0, 0, 2 }, 0.0f,
					 nobody, &hit));
	check_hit(hit, base, 0.0f, (voe_math_float3){ 0.5f, 0, 0 },
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
		a_ray_hits_a_turned_box(arena, places[i]);
		a_sphere_at_corners_and_edges(arena, places[i]);
		a_thin_box_is_not_tunnelled(arena, places[i]);
		a_child_box_is_hit_at_its_world_place(arena, places[i]);
		a_ray_starts_inside_a_box(arena, places[i]);
	}

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
