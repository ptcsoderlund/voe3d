// The water system: that a water gains a waves row after one run, the clock
// after three half seconds is 1.5, 61 s of steps wrap it to 1, a replace
// changes the row's colour, and a removed water's waves row is gone after the
// next run. Needs no graphics card.
#include <3d/water_system.h>

#include <base/arena.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>

#define SCRATCH (64 * 1024)
#define ENTITIES 4

static voe_ecs_world *a_world(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = ENTITIES,
		.component_types = 3,
		.intent_types = 2,
		.structure_requests = 4 * ENTITIES,
		.structure_bytes = 1024,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_transform_register(world, ENTITIES);
	voe_3d_water_register(world, ENTITIES);
	return world;
}

static voe_ecs_entity a_water(voe_ecs_world *world)
{
	voe_ecs_entity entity = { 0 };
	voe_ecs_type water = voe_ecs_component_type(world, &voe_3d_water_key);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_3d_water_add(
		world, entity,
		*(const voe_3d_water *)voe_ecs_component_default(world, water)));
	return entity;
}

static double clock_of(const voe_ecs_world *world, voe_ecs_entity entity)
{
	const voe_3d_waves *waves = voe_3d_waves_get(world, entity);

	VOE_TEST_CHECK(waves != NULL);
	return waves != NULL ? waves->seconds : -1.0;
}

static void a_water_gains_a_waves_row(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity entity = a_water(world);

	VOE_TEST_CHECK(voe_3d_waves_get(world, entity) == NULL);
	voe_3d_water_system_run(world, 0.0f);
	VOE_TEST_CHECK_INT(voe_3d_waves_count(world), 1);
	VOE_TEST_CHECK(clock_of(world, entity) == 0.0);
	voe_base_arena_clear(arena);
}

static void the_clock_steps_and_wraps(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity entity = a_water(world);

	for (int i = 0; i < 3; i++)
		voe_3d_water_system_run(world, 0.5f);
	VOE_TEST_CHECK(clock_of(world, entity) == 1.5);
	voe_base_arena_clear(arena);

	// 122 half seconds, exact in float, so the wrap alone is measured.
	world = a_world(arena);
	entity = a_water(world);
	for (int i = 0; i < 122; i++)
		voe_3d_water_system_run(world, 0.5f);
	VOE_TEST_CHECK(fabs(clock_of(world, entity) - 1.0) < 1e-9);
	voe_base_arena_clear(arena);
}

static void a_replace_changes_the_colour(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity entity = a_water(world);
	voe_3d_water row = *voe_3d_water_get(world, entity);

	row.colour = (voe_math_float3){ 1.0f, 0.0f, 0.0f };
	VOE_TEST_CHECK(voe_3d_water_submit(
		world, (voe_3d_water_intent){ .entity = entity, .water = row }));
	voe_3d_water_system_run(world, 0.0f);
	VOE_TEST_CHECK(voe_3d_water_get(world, entity)->colour.x == 1.0f);
	VOE_TEST_CHECK(voe_3d_water_get(world, entity)->colour.y == 0.0f);
	voe_base_arena_clear(arena);
}

static void a_removed_waters_row_is_gone(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity entity = a_water(world);

	voe_3d_water_system_run(world, 0.0f);
	VOE_TEST_CHECK(voe_3d_waves_get(world, entity) != NULL);
	VOE_TEST_CHECK(voe_ecs_component_remove(
		world, voe_ecs_component_type(world, &voe_3d_water_key), entity));
	voe_3d_water_system_run(world, 0.0f);
	VOE_TEST_CHECK(voe_3d_waves_get(world, entity) == NULL);
	VOE_TEST_CHECK_INT(voe_3d_waves_count(world), 0);
	voe_base_arena_clear(arena);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);

	a_water_gains_a_waves_row(arena);
	the_clock_steps_and_wraps(arena);
	a_replace_changes_the_colour(arena);
	a_removed_waters_row_is_gone(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
