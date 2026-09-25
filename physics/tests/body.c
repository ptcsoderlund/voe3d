// Kinematic bodies: registered after colliders, the default row and what the
// type needs, a row replaced whole when the system runs, and each bad field
// keeping the row.
//
// THE REFUSED ROWS ARE REPORTED ON stderr. Those lines are expected output, not
// a failure; the checks are on the row.
#include <base/arena.h>
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

#define ENTITIES 4

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

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);

	registration_tells_a_tool(arena);
	a_row_is_replaced_whole_and_kept(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
