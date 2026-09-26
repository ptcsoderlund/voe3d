// Kinematic bodies: registered after colliders, the default row and what the
// type needs, a row replaced whole when the system runs, each bad field
// keeping the row, and the move: resting, walls, ramps, a ledge and a gap.
//
// THE REFUSED ROWS ARE REPORTED ON stderr. Those lines are expected output, not
// a failure; the checks are on the row.
//
// GRAVITY IS THE TEST'S OWN, as it is a project's (0249): each step it adds a
// fall to the velocity it submits, from rest when the body is on the floor.
//
// EVERY MOVING SCENE RUNS TWICE, at the origin and moved by (100000, 0,
// 100000), and ends at the same place to 1e-4 (0250).
#include <base/arena.h>
#include <math/double3.h>
#include <math/quat.h>
#include <ecs/component.h>
#include <ecs/world.h>
#include <physics/body_component.h>
#include <physics/body_system.h>
#include <physics/collider_component.h>
#include <physics/collider_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>
#include <string.h>

#define ENTITIES 8
#define STEP (1.0f / 60.0f)
#define GRAVITY 9.81f
#define DEGREES (3.14159265f / 180.0f)

static voe_ecs_world *world_of(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = ENTITIES,
		.component_types = 3,
		.intent_types = 3,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_transform_register(world, ENTITIES);
	voe_physics_collider_register(world, ENTITIES);
	voe_physics_body_register(world, ENTITIES);
	return world;
}

// The default row is a step of 0.3, a slope of 0.8, still and not on the
// floor; the type needs a collider and sits at "Physics / Kinematic Body".
static void registration_tells_a_tool(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type type = voe_ecs_component_type(world, &voe_physics_body_key);
	voe_ecs_type needed = { 0 };
	const voe_physics_body *row = voe_ecs_component_default(world, type);

	VOE_TEST_CHECK(row != NULL);
	if (row != NULL) {
		VOE_TEST_CHECK_FLOAT(row->step_height, 0.3f, 0.0f);
		VOE_TEST_CHECK_FLOAT(row->slope_limit, 0.8f, 0.0f);
		VOE_TEST_CHECK_FLOAT(row->velocity.x, 0.0f, 0.0f);
		VOE_TEST_CHECK_FLOAT(row->velocity.y, 0.0f, 0.0f);
		VOE_TEST_CHECK_FLOAT(row->velocity.z, 0.0f, 0.0f);
		VOE_TEST_CHECK(!row->on_floor);
	}
	VOE_TEST_CHECK(voe_ecs_component_needs(world, type, &needed));
	VOE_TEST_CHECK_INT(needed.value,
			   voe_ecs_component_type(world,
						  &voe_physics_collider_key)
				   .value);
	VOE_TEST_CHECK(strcmp(voe_ecs_component_menu(world, type),
			      "Physics / Kinematic Body") == 0);
}

// Submits `body` for `entity` and runs the drain.
static void drain(voe_ecs_world *world, voe_ecs_entity entity,
		  voe_physics_body body)
{
	VOE_TEST_CHECK(voe_physics_body_submit(
		world,
		(voe_physics_body_intent){ .entity = entity, .body = body }));
	voe_physics_body_system_run(world);
}

// Field by field, since the row's padding is not part of it.
static bool same(const voe_physics_body *row, voe_physics_body want)
{
	return row != NULL && row->step_height == want.step_height &&
	       row->slope_limit == want.slope_limit &&
	       row->velocity.x == want.velocity.x &&
	       row->velocity.y == want.velocity.y &&
	       row->velocity.z == want.velocity.z &&
	       row->on_floor == want.on_floor;
}

// A replace lands whole when the system runs; each bad field keeps that row.
static void a_row_is_replaced_whole_and_kept(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity entity = { 0 };
	voe_physics_body good = { .step_height = 0.5f,
				  .slope_limit = 1.0f,
				  .velocity = { 1.0f, -2.0f, 3.0f },
				  .on_floor = true };
	voe_physics_body bad[] = { good, good, good, good, good };
	const voe_physics_body *row;

	bad[0].step_height = -0.1f;
	bad[1].step_height = INFINITY;
	bad[2].slope_limit = 1.6f;
	bad[3].slope_limit = -0.1f;
	bad[4].velocity.y = NAN;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_physics_body_add(
		world, entity,
		(voe_physics_body){ .step_height = 0.3f, .slope_limit = 0.8f }));
	VOE_TEST_CHECK_INT(voe_physics_body_count(world), 1);

	VOE_TEST_CHECK(voe_physics_body_submit(
		world,
		(voe_physics_body_intent){ .entity = entity, .body = good }));
	row = voe_physics_body_get(world, entity);
	VOE_TEST_CHECK(row != NULL && row->step_height == 0.3f);
	voe_physics_body_system_run(world);
	row = voe_physics_body_get(world, entity);
	VOE_TEST_CHECK(same(row, good));

	for (uint32_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
		drain(world, entity, bad[i]);
		VOE_TEST_CHECK(same(voe_physics_body_get(world, entity), good));
	}

	// A drain that settles nothing closes the run the refusals opened.
	voe_physics_body_system_run(world);
}

static voe_math_quat turned_about_z(float angle)
{
	return voe_math_quat_from_axis_angle((voe_math_float3){ 0, 0, 1 },
					     angle);
}

// An entity at `base` + `at` with a collider of `kind` and `size`.
static voe_ecs_entity placed(voe_ecs_world *world, voe_math_double3 base,
			     voe_math_double3 at, float angle, uint32_t kind,
			     voe_math_float3 size)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){
			.position = { base.x + at.x, base.y + at.y,
				      base.z + at.z },
			.rotation = turned_about_z(angle),
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_physics_collider_add(
		world, entity,
		(voe_physics_collider){ .kind = kind, .size = size }));
	return entity;
}

static void box(voe_ecs_world *world, voe_math_double3 base,
		voe_math_double3 at, float angle, voe_math_float3 size)
{
	(void)placed(world, base, at, angle, VOE_PHYSICS_COLLIDER_BOX, size);
}

// A floor whose top is y = 0 and whose far edge is at x = `edge`.
static void floor_to(voe_ecs_world *world, voe_math_double3 base, double edge)
{
	box(world, base, (voe_math_double3){ edge - 20.0, -0.5, 0.0 }, 0.0f,
	    (voe_math_float3){ 40.0f, 1.0f, 40.0f });
}

// A capsule (1, 2, 1) body standing at x = 0, its centre `height` up.
static voe_ecs_entity player(voe_ecs_world *world, voe_math_double3 base,
			     double height)
{
	voe_ecs_entity body = placed(world, base,
				     (voe_math_double3){ 0.0, height, 0.0 },
				     0.0f, VOE_PHYSICS_COLLIDER_CAPSULE,
				     (voe_math_float3){ 1.0f, 2.0f, 1.0f });

	VOE_TEST_CHECK(voe_physics_body_add(
		world, body,
		(voe_physics_body){ .step_height = 0.3f, .slope_limit = 0.8f }));
	return body;
}

// A ramp rising along +x at `angle`, its top meeting the floor at x = 1.
static void ramp(voe_ecs_world *world, voe_math_double3 base, float angle)
{
	double s = sin((double)angle);
	double c = cos((double)angle);

	box(world, base,
	    (voe_math_double3){ 1.0 + 4.0 * c + 0.5 * s, 4.0 * s - 0.5 * c,
				0.0 },
	    angle, (voe_math_float3){ 10.0f, 1.0f, 10.0f });
}

// `steps` fixed steps wanting (x, 0, z), with the test's own gravity.
static void walk(voe_ecs_world *world, voe_ecs_entity body, float x, float z,
		 uint32_t steps)
{
	for (uint32_t i = 0; i < steps; i++) {
		voe_physics_body row = *voe_physics_body_get(world, body);

		row.velocity.y = row.on_floor ? -GRAVITY * STEP :
						row.velocity.y - GRAVITY * STEP;
		row.velocity.x = x;
		row.velocity.z = z;
		drain(world, body, row);
		voe_physics_body_system_move(world, STEP);
		voe_scene_transform_system_run(world);
	}
}

// Where the body is, relative to the scene's base.
static voe_math_double3 at(voe_ecs_world *world, voe_ecs_entity body,
			   voe_math_double3 base)
{
	return voe_math_double3_sub(voe_scene_transform_get(world, body)->position,
				    base);
}

static bool on_floor(voe_ecs_world *world, voe_ecs_entity body)
{
	return voe_physics_body_get(world, body)->on_floor;
}

// Dropped half a metre, it lands and rests: still over the last second.
static voe_math_double3 rests(voe_base_arena *arena, voe_math_double3 base)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity body;
	double settled;
	double most = 0.0;

	floor_to(world, base, 20.0);
	body = player(world, base, 1.5);
	walk(world, body, 0.0f, 0.0f, 60);
	settled = at(world, body, base).y;
	for (uint32_t i = 0; i < 60; i++) {
		walk(world, body, 0.0f, 0.0f, 1);
		most = fmax(most, fabs(at(world, body, base).y - settled));
	}
	VOE_TEST_CHECK(most < 1e-4);
	VOE_TEST_CHECK_FLOAT((float)settled, 1.0f, 1e-3f);
	VOE_TEST_CHECK(on_floor(world, body));
	return at(world, body, base);
}

// A wall whose face is x = 2: head on it stops touching, at 45° it slides.
static voe_math_double3 at_a_wall(voe_base_arena *arena, voe_math_double3 base,
				  bool slanted)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity body;
	voe_math_double3 end;

	floor_to(world, base, 20.0);
	box(world, base, (voe_math_double3){ 2.5, 1.5, 0.0 }, 0.0f,
	    (voe_math_float3){ 1.0f, 3.0f, 20.0f });
	body = player(world, base, 1.0);
	walk(world, body, slanted ? 1.41421356f : 2.0f,
	     slanted ? 1.41421356f : 0.0f, 120);
	end = at(world, body, base);
	VOE_TEST_CHECK_FLOAT((float)end.x, 1.5f, 1e-3f);
	VOE_TEST_CHECK_FLOAT((float)end.y, 1.0f, 1e-3f);
	VOE_TEST_CHECK(slanted ? end.z > 1.5 : fabs(end.z) < 1e-4);
	VOE_TEST_CHECK(on_floor(world, body));
	return end;
}

static voe_math_double3 head_on(voe_base_arena *arena, voe_math_double3 base)
{
	return at_a_wall(arena, base, false);
}

static voe_math_double3 slides(voe_base_arena *arena, voe_math_double3 base)
{
	return at_a_wall(arena, base, true);
}

// Walked into a ramp: 15° is climbed, 55° is not.
static voe_math_double3 up_a_ramp(voe_base_arena *arena, voe_math_double3 base,
				  float degrees)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity body;
	voe_math_double3 end;

	floor_to(world, base, 20.0);
	ramp(world, base, degrees * DEGREES);
	body = player(world, base, 1.0);
	walk(world, body, 2.0f, 0.0f, 120);
	end = at(world, body, base);
	if (degrees < 45.0f) {
		VOE_TEST_CHECK(end.y > 1.5);
		VOE_TEST_CHECK(on_floor(world, body));
	} else {
		VOE_TEST_CHECK(end.y < 1.05);
	}
	return end;
}

static voe_math_double3 gentle(voe_base_arena *arena, voe_math_double3 base)
{
	return up_a_ramp(arena, base, 15.0f);
}

static voe_math_double3 steep(voe_base_arena *arena, voe_math_double3 base)
{
	return up_a_ramp(arena, base, 55.0f);
}

// A 0.25 m ledge from x = 2 is stepped onto, walking only.
static voe_math_double3 onto_a_ledge(voe_base_arena *arena,
				     voe_math_double3 base)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity body;
	voe_math_double3 end;

	floor_to(world, base, 20.0);
	box(world, base, (voe_math_double3){ 6.0, 0.125, 0.0 }, 0.0f,
	    (voe_math_float3){ 8.0f, 0.25f, 20.0f });
	body = player(world, base, 1.0);
	walk(world, body, 2.0f, 0.0f, 120);
	end = at(world, body, base);
	VOE_TEST_CHECK(end.x > 3.0);
	VOE_TEST_CHECK_FLOAT((float)end.y, 1.25f, 1e-2f);
	VOE_TEST_CHECK(on_floor(world, body));
	return end;
}

// Walked off the floor's edge at x = 2 over nothing, it falls.
static voe_math_double3 off_an_edge(voe_base_arena *arena,
				    voe_math_double3 base)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity body;
	voe_math_double3 end;

	floor_to(world, base, 2.0);
	body = player(world, base, 1.0);
	walk(world, body, 2.0f, 0.0f, 120);
	end = at(world, body, base);
	VOE_TEST_CHECK(end.y < 0.0);
	VOE_TEST_CHECK(!on_floor(world, body));
	return end;
}

static voe_math_double3 (*const scenes[])(voe_base_arena *,
					  voe_math_double3) = {
	rests, head_on, slides, gentle, steep, onto_a_ledge, off_an_edge,
};

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(4 * 1024 * 1024);
	const voe_math_double3 far = { 100000.0, 0.0, 100000.0 };

	registration_tells_a_tool(arena);
	a_row_is_replaced_whole_and_kept(arena);
	for (uint32_t i = 0; i < sizeof scenes / sizeof scenes[0]; i++) {
		voe_math_double3 near_end = scenes[i](arena, (voe_math_double3){ 0 });
		voe_math_double3 far_end = scenes[i](arena, far);

		VOE_TEST_CHECK_FLOAT((float)(far_end.x - near_end.x), 0.0f, 1e-4f);
		VOE_TEST_CHECK_FLOAT((float)(far_end.y - near_end.y), 0.0f, 1e-4f);
		VOE_TEST_CHECK_FLOAT((float)(far_end.z - near_end.z), 0.0f, 1e-4f);
	}

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
