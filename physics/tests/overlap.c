// The overlap query: a capsule on a floor, against a wall and on a tilted box;
// sphere against sphere and capsule against capsule; a trigger flagged, the
// ignored entity left out, and capacity honoured.
//
// EVERY SCENE RUNS TWICE, at the origin and moved by (100000, 0, 100000), and
// checks the same numbers to 1e-5. A float there has a step of about 0.008, so
// only a query that subtracts centres in double first passes both (0250).
#include <base/arena.h>
#include <ecs/world.h>
#include <math/quat.h>
#include <physics/collider_component.h>
#include <physics/collider_system.h>
#include <physics/overlap.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>

#define ENTITIES 4
#define ANGLE 0.34906585f // 20 degrees

static voe_ecs_world *world_of(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = ENTITIES,
		.component_types = 2,
		.intent_types = 2,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_transform_register(world, ENTITIES);
	voe_physics_collider_register(world, ENTITIES);
	return world;
}

static voe_math_quat turned_about_z(float angle)
{
	return voe_math_quat_from_axis_angle((voe_math_float3){ 0, 0, 1 },
					     angle);
}

// An entity with a collider of `kind` and `size` at `base` + (x, y, z).
static voe_ecs_entity placed(voe_ecs_world *world, voe_math_double3 base,
			     voe_math_double3 at, voe_math_quat rotation,
			     uint32_t kind, voe_math_float3 size, bool trigger)
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

// The player's capsule: radius a half, whole height two, upright.
static voe_physics_shape capsule_at(voe_math_double3 base, double y)
{
	return (voe_physics_shape){ .kind = VOE_PHYSICS_COLLIDER_CAPSULE,
				    .centre = { base.x, base.y + y, base.z },
				    .rotation = turned_about_z(0.0f),
				    .half = { 0.5f, 1.0f, 0.0f } };
}

static void check_contact(voe_physics_contact contact, voe_math_float3 normal,
			  float depth)
{
	VOE_TEST_CHECK_FLOAT(contact.normal.x, normal.x, 1e-5f);
	VOE_TEST_CHECK_FLOAT(contact.normal.y, normal.y, 1e-5f);
	VOE_TEST_CHECK_FLOAT(contact.normal.z, normal.z, 1e-5f);
	VOE_TEST_CHECK_FLOAT(contact.depth, depth, 1e-5f);
}

// A floor whose top is y = 0, a capsule sunk into it 1 cm.
static void a_capsule_on_a_floor(voe_base_arena *arena, voe_math_double3 base)
{
	voe_ecs_world *world = world_of(arena);
	voe_physics_contact contacts[4];
	voe_ecs_entity floor =
		placed(world, base, (voe_math_double3){ 0.0, -0.5, 0.0 },
		       turned_about_z(0.0f), VOE_PHYSICS_COLLIDER_BOX,
		       (voe_math_float3){ 10.0f, 1.0f, 10.0f }, false);
	uint32_t n = voe_physics_overlap(world, capsule_at(base, 0.99),
					 (voe_ecs_entity){ 0 }, contacts, 4);

	VOE_TEST_CHECK_INT(n, 1);
	if (n == 1) {
		VOE_TEST_CHECK(contacts[0].entity.index == floor.index);
		VOE_TEST_CHECK(!contacts[0].trigger);
		check_contact(contacts[0], (voe_math_float3){ 0, 1, 0 }, 0.01f);
	}
}

// A wall whose face is x = 0.49, the capsule's side at x = 0.5.
static void a_capsule_against_a_wall(voe_base_arena *arena,
				     voe_math_double3 base)
{
	voe_ecs_world *world = world_of(arena);
	voe_physics_contact contacts[4];
	uint32_t n;

	(void)placed(world, base, (voe_math_double3){ 0.99, 0.0, 0.0 },
		     turned_about_z(0.0f), VOE_PHYSICS_COLLIDER_BOX,
		     (voe_math_float3){ 1.0f, 4.0f, 4.0f }, false);
	n = voe_physics_overlap(world, capsule_at(base, 0.0),
				(voe_ecs_entity){ 0 }, contacts, 4);
	VOE_TEST_CHECK_INT(n, 1);
	if (n == 1)
		check_contact(contacts[0], (voe_math_float3){ -1, 0, 0 }, 0.01f);
}

// A slab turned 20 degrees about Z, the capsule's foot sunk 1 cm into its top
// face: pushed out along that face's normal.
static void a_capsule_on_a_tilted_box(voe_base_arena *arena,
				      voe_math_double3 base)
{
	voe_ecs_world *world = world_of(arena);
	voe_physics_contact contacts[4];
	const double foot = (1.0 - 0.01) / cos((double)ANGLE);
	uint32_t n;

	(void)placed(world, base, (voe_math_double3){ 0.0, 0.0, 0.0 },
		     turned_about_z(ANGLE), VOE_PHYSICS_COLLIDER_BOX,
		     (voe_math_float3){ 10.0f, 1.0f, 10.0f }, false);
	n = voe_physics_overlap(world, capsule_at(base, foot + 0.5),
				(voe_ecs_entity){ 0 }, contacts, 4);
	VOE_TEST_CHECK_INT(n, 1);
	if (n == 1)
		check_contact(contacts[0],
			      (voe_math_float3){ -sinf(ANGLE), cosf(ANGLE), 0 },
			      0.01f);
}

// A sphere of radius 1 half a metre into another; a capsule lying along X
// with its segment 0.9 above the upright one's top.
static void round_against_round(voe_base_arena *arena, voe_math_double3 base)
{
	voe_ecs_world *world = world_of(arena);
	voe_physics_contact contacts[4];
	voe_physics_shape sphere = { .kind = VOE_PHYSICS_COLLIDER_SPHERE,
				     .centre = base,
				     .rotation = turned_about_z(0.0f),
				     .half = { 1.0f, 0.0f, 0.0f } };
	uint32_t n;

	(void)placed(world, base, (voe_math_double3){ 1.5, 0.0, 0.0 },
		     turned_about_z(0.0f), VOE_PHYSICS_COLLIDER_SPHERE,
		     (voe_math_float3){ 2.0f, 2.0f, 2.0f }, false);
	(void)placed(world, base, (voe_math_double3){ 0.0, 1.4, 20.0 },
		     turned_about_z(1.5707964f), VOE_PHYSICS_COLLIDER_CAPSULE,
		     (voe_math_float3){ 1.0f, 2.0f, 1.0f }, false);

	n = voe_physics_overlap(world, sphere, (voe_ecs_entity){ 0 }, contacts,
				4);
	VOE_TEST_CHECK_INT(n, 1);
	if (n == 1)
		check_contact(contacts[0], (voe_math_float3){ -1, 0, 0 }, 0.5f);

	n = voe_physics_overlap(
		world,
		capsule_at((voe_math_double3){ base.x, base.y, base.z + 20.0 },
			   0.0),
		(voe_ecs_entity){ 0 }, contacts, 4);
	VOE_TEST_CHECK_INT(n, 1);
	if (n == 1)
		check_contact(contacts[0], (voe_math_float3){ 0, -1, 0 }, 0.1f);
}

// Two spheres overlap the query, one a trigger: both found and the trigger
// flagged; ignoring one leaves the other; capacity 1 writes one.
static void trigger_ignore_and_capacity(voe_base_arena *arena,
					voe_math_double3 base)
{
	voe_ecs_world *world = world_of(arena);
	voe_physics_contact contacts[4];
	voe_ecs_entity solid =
		placed(world, base, (voe_math_double3){ 0.5, 0.0, 0.0 },
		       turned_about_z(0.0f), VOE_PHYSICS_COLLIDER_SPHERE,
		       (voe_math_float3){ 1.0f, 1.0f, 1.0f }, false);
	voe_ecs_entity trigger =
		placed(world, base, (voe_math_double3){ -0.5, 0.0, 0.0 },
		       turned_about_z(0.0f), VOE_PHYSICS_COLLIDER_SPHERE,
		       (voe_math_float3){ 1.0f, 1.0f, 1.0f }, true);
	voe_physics_shape query = capsule_at(base, 0.0);
	uint32_t n;

	n = voe_physics_overlap(world, query, (voe_ecs_entity){ 0 }, contacts, 4);
	VOE_TEST_CHECK_INT(n, 2);
	if (n == 2) {
		VOE_TEST_CHECK(!contacts[0].trigger);
		VOE_TEST_CHECK(contacts[1].trigger);
		VOE_TEST_CHECK(contacts[1].entity.index == trigger.index);
	}

	n = voe_physics_overlap(world, query, solid, contacts, 4);
	VOE_TEST_CHECK_INT(n, 1);
	if (n == 1)
		VOE_TEST_CHECK(contacts[0].entity.index == trigger.index);

	n = voe_physics_overlap(world, query, (voe_ecs_entity){ 0 }, contacts, 1);
	VOE_TEST_CHECK_INT(n, 1);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);
	const voe_math_double3 places[2] = { { 0.0, 0.0, 0.0 },
					     { 100000.0, 0.0, 100000.0 } };

	for (int i = 0; i < 2; i++) {
		a_capsule_on_a_floor(arena, places[i]);
		a_capsule_against_a_wall(arena, places[i]);
		a_capsule_on_a_tilted_box(arena, places[i]);
		round_against_round(arena, places[i]);
		trigger_ignore_and_capacity(arena, places[i]);
	}

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
