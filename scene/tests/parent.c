// A tank's hull, turret and barrel: that the barrel's world place follows the
// hull moved and turned, that a world with no parent table answers the row, that
// within and tree walk the chain both ways, that a loop written by hand ends the
// walk instead of hanging, and that local then world round-trips.
//
// THE ROWS ARE WRITTEN STRAIGHT WITH voe_ecs_component_add, because parenting
// through the structural queue is card 03's and what is checked here is the
// reads over rows that exist.
#include <base/arena.h>
#include <ecs/component.h>
#include <ecs/world.h>
#include <math/quat.h>
#include <scene/parent_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>

#define TOLERANCE 1e-5f
#define ENTITIES 8
#define QUARTER_TURN 1.57079632679489661923f

struct tank {
	voe_ecs_world *world;
	voe_ecs_entity hull, turret, barrel;
};

static voe_ecs_world *world_of(voe_base_arena *arena, bool parents)
{
	voe_ecs_world *world = voe_ecs_world_new(
		arena, (voe_ecs_limits){ .entities = ENTITIES,
					 .component_types = 3,
					 .intent_types = 2 });

	voe_scene_transform_register(world, ENTITIES);
	if (parents)
		voe_scene_parent_register(world, ENTITIES);
	return world;
}

static voe_scene_transform at(double x, double y, double z)
{
	return (voe_scene_transform){ .position = { x, y, z },
				      .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				      .scale = { 1.0f, 1.0f, 1.0f } };
}

static voe_ecs_entity placed(voe_ecs_world *world, voe_scene_transform row)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, row));
	return entity;
}

static void hang(voe_ecs_world *world, voe_ecs_entity child,
		 voe_ecs_entity parent)
{
	VOE_TEST_CHECK(voe_ecs_component_add(
		world, voe_ecs_component_type(world, &voe_scene_parent_key),
		child, &(voe_scene_parent){ .parent = parent }));
}

static struct tank tank_of(voe_base_arena *arena, bool parents)
{
	struct tank tank = { .world = world_of(arena, parents) };

	tank.hull = placed(tank.world, at(10.0, 0.0, 0.0));
	tank.turret = placed(tank.world, at(0.0, 1.0, 0.0));
	tank.barrel = placed(tank.world, at(0.0, 0.0, -2.0));
	if (parents) {
		hang(tank.world, tank.turret, tank.hull);
		hang(tank.world, tank.barrel, tank.turret);
	}
	return tank;
}

static void check_position(voe_scene_transform t, double x, double y, double z)
{
	VOE_TEST_CHECK_FLOAT(t.position.x, x, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(t.position.y, y, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(t.position.z, z, TOLERANCE);
}

// A quarter turn of the hull about +Y takes the barrel's -Z to -X.
static void the_barrel_follows_the_hull(voe_base_arena *arena)
{
	struct tank tank = tank_of(arena, true);
	voe_scene_transform hull = at(10.0, 0.0, 5.0);

	check_position(voe_scene_transform_world(tank.world, tank.barrel), 10.0,
		       1.0, -2.0);

	hull.rotation = voe_math_quat_from_axis_angle(
		(voe_math_float3){ 0.0f, 1.0f, 0.0f }, QUARTER_TURN);
	VOE_TEST_CHECK(voe_scene_transform_submit(
		tank.world, (voe_scene_transform_intent){ .entity = tank.hull,
							  .transform = hull }));
	voe_scene_transform_system_run(tank.world);

	check_position(voe_scene_transform_world(tank.world, tank.barrel), 8.0,
		       1.0, 5.0);
}

static void no_table_answers_the_row(voe_base_arena *arena)
{
	struct tank tank = tank_of(arena, false);

	check_position(voe_scene_transform_world(tank.world, tank.barrel), 0.0,
		       0.0, -2.0);
	VOE_TEST_CHECK(voe_scene_parent_get(tank.world, tank.barrel) == NULL);
	VOE_TEST_CHECK(!voe_scene_parent_within(tank.world, tank.barrel,
						tank.hull));
}

static void within_walks_up_only(voe_base_arena *arena)
{
	struct tank tank = tank_of(arena, true);

	VOE_TEST_CHECK(voe_scene_parent_within(tank.world, tank.barrel,
					       tank.hull));
	VOE_TEST_CHECK(voe_scene_parent_within(tank.world, tank.barrel,
					       tank.barrel));
	VOE_TEST_CHECK(!voe_scene_parent_within(tank.world, tank.hull,
						tank.barrel));
}

static void the_tree_lists_the_hull_first(voe_base_arena *arena)
{
	struct tank tank = tank_of(arena, true);
	voe_ecs_entity out[ENTITIES];
	uint32_t count = voe_scene_parent_tree(tank.world, tank.hull, out,
					       ENTITIES);

	VOE_TEST_CHECK_INT(count, 3);
	VOE_TEST_CHECK_INT(out[0].index, tank.hull.index);
	VOE_TEST_CHECK_INT(out[1].index, tank.turret.index);
	VOE_TEST_CHECK_INT(out[2].index, tank.barrel.index);
	VOE_TEST_CHECK_INT(voe_scene_parent_tree(tank.world, tank.hull, out, 2),
			   2);
	VOE_TEST_CHECK_INT(voe_scene_parent_tree(tank.world, tank.barrel, out,
						 ENTITIES),
			   1);
}

// Each is the other's parent: 32 links of one metre each way, and then it stops.
static void a_loop_ends_the_walk(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena, true);
	voe_ecs_entity a = placed(world, at(1.0, 0.0, 0.0));
	voe_ecs_entity b = placed(world, at(1.0, 0.0, 0.0));
	voe_ecs_entity out[ENTITIES];

	hang(world, a, b);
	hang(world, b, a);
	check_position(voe_scene_transform_world(world, a),
		       VOE_SCENE_PARENT_DEPTH_MAX + 1, 0.0, 0.0);
	VOE_TEST_CHECK(!voe_scene_parent_within(world, a, (voe_ecs_entity){ 0 }));
	VOE_TEST_CHECK_INT(voe_scene_parent_tree(world, a, out, ENTITIES),
			   ENTITIES);
}

static void local_then_world_round_trips(voe_base_arena *arena)
{
	struct tank tank = tank_of(arena, true);
	voe_scene_transform wanted = at(3.0, 4.0, 5.0);
	voe_scene_transform row;

	wanted.rotation = voe_math_quat_from_axis_angle(
		(voe_math_float3){ 1.0f, 0.0f, 0.0f }, 0.5f);
	row = voe_scene_transform_local(tank.world, tank.barrel, wanted);
	VOE_TEST_CHECK(voe_scene_transform_submit(
		tank.world, (voe_scene_transform_intent){ .entity = tank.barrel,
							  .transform = row }));
	voe_scene_transform_system_run(tank.world);

	check_position(voe_scene_transform_world(tank.world, tank.barrel), 3.0,
		       4.0, 5.0);
	VOE_TEST_CHECK_FLOAT(
		voe_scene_transform_world(tank.world, tank.barrel).rotation.x,
		wanted.rotation.x, TOLERANCE);
	check_position(voe_scene_transform_local(tank.world, tank.hull, wanted),
		       3.0, 4.0, 5.0);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(256 * 1024);

	the_barrel_follows_the_hull(arena);
	no_table_answers_the_row(arena);
	within_walks_up_only(arena);
	the_tree_lists_the_hull_first(arena);
	a_loop_ends_the_walk(arena);
	local_then_world_round_trips(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
