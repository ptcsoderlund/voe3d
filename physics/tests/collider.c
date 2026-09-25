// Colliders: what registration tells a tool, that an intent lands only when the
// system runs and a bad size keeps the row, and what shape a collider becomes in
// the world once its transform's scale applies.
//
// THE BOX SITS AT x = 100000 ON PURPOSE. A float there has a step of about
// 0.008, so a centre that passed through a float would come back off by up to
// that; the check is for exact equality, which only a double survives (0250).
//
// THE BAD SIZE IS REPORTED ON stderr. That line is expected output, not a
// failure; the checks are on the row.
#include <base/arena.h>
#include <ecs/component.h>
#include <ecs/world.h>
#include <math/quat.h>
#include <physics/collider_component.h>
#include <physics/collider_system.h>
#include <physics/shape.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <string.h>

#define ENTITIES 4

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

// An entity with a transform at `position` scaled by `scale`, and `collider`.
static voe_ecs_entity placed(voe_ecs_world *world, voe_math_double3 position,
			     voe_math_float3 scale,
			     voe_physics_collider collider)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .position = position,
				       .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = scale }));
	VOE_TEST_CHECK(voe_physics_collider_add(world, entity, collider));
	return entity;
}

// The default row is a box of 1, not a trigger; the type needs a transform and
// sits at "Physics / Collider".
static void registration_tells_a_tool(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type type =
		voe_ecs_component_type(world, &voe_physics_collider_key);
	const voe_physics_collider *row = voe_ecs_component_default(world, type);

	VOE_TEST_CHECK(row != NULL);
	if (row != NULL) {
		VOE_TEST_CHECK_INT(row->kind, VOE_PHYSICS_COLLIDER_BOX);
		VOE_TEST_CHECK_FLOAT(row->size.x, 1.0f, 0.0f);
		VOE_TEST_CHECK_FLOAT(row->size.y, 1.0f, 0.0f);
		VOE_TEST_CHECK_FLOAT(row->size.z, 1.0f, 0.0f);
		VOE_TEST_CHECK(!row->trigger);
	}
	VOE_TEST_CHECK(strcmp(voe_ecs_component_menu(world, type),
			      "Physics / Collider") == 0);
	VOE_TEST_CHECK(strcmp(voe_physics_collider_kind_names
				      [VOE_PHYSICS_COLLIDER_CAPSULE],
			      "Capsule") == 0);
}

// Add is read back at once; a replace lands only when the system runs; a
// negative size keeps the row.
static void an_intent_is_drained_and_a_bad_size_kept(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_physics_collider sphere = { .kind = VOE_PHYSICS_COLLIDER_SPHERE,
					.size = { 2.0f, 2.0f, 2.0f },
					.trigger = true };
	voe_ecs_entity entity =
		placed(world, (voe_math_double3){ 0.0, 0.0, 0.0 },
		       (voe_math_float3){ 1.0f, 1.0f, 1.0f },
		       (voe_physics_collider){ .kind = VOE_PHYSICS_COLLIDER_BOX,
					       .size = { 1.0f, 1.0f, 1.0f } });
	const voe_physics_collider *row;

	VOE_TEST_CHECK_INT(voe_physics_collider_count(world), 1);
	VOE_TEST_CHECK(voe_physics_collider_submit(
		world, (voe_physics_collider_intent){ .entity = entity,
						      .collider = sphere }));
	row = voe_physics_collider_get(world, entity);
	VOE_TEST_CHECK(row != NULL && row->kind == VOE_PHYSICS_COLLIDER_BOX);

	voe_physics_collider_system_run(world);
	row = voe_physics_collider_get(world, entity);
	VOE_TEST_CHECK(row != NULL && row->kind == VOE_PHYSICS_COLLIDER_SPHERE);
	VOE_TEST_CHECK(row != NULL && row->trigger);

	sphere.size.y = -1.0f;
	sphere.kind = VOE_PHYSICS_COLLIDER_CAPSULE;
	VOE_TEST_CHECK(voe_physics_collider_submit(
		world, (voe_physics_collider_intent){ .entity = entity,
						      .collider = sphere }));
	voe_physics_collider_system_run(world);
	row = voe_physics_collider_get(world, entity);
	VOE_TEST_CHECK(row != NULL && row->kind == VOE_PHYSICS_COLLIDER_SPHERE);
	VOE_TEST_CHECK(row != NULL && row->size.y == 2.0f);

	// A drain that settles nothing closes the run the bad size opened.
	voe_physics_collider_system_run(world);
}

// A flat box far from the origin: half its scaled size, and the centre exact.
static void a_scaled_box_far_away(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_physics_shape shape = { 0 };
	voe_ecs_entity entity =
		placed(world, (voe_math_double3){ 100000.0, 0.0, 0.0 },
		       (voe_math_float3){ 10.0f, 0.1f, 10.0f },
		       (voe_physics_collider){ .kind = VOE_PHYSICS_COLLIDER_BOX,
					       .size = { 1.0f, 1.0f, 1.0f } });

	VOE_TEST_CHECK(voe_physics_shape_of(world, entity, &shape));
	VOE_TEST_CHECK_INT(shape.kind, VOE_PHYSICS_COLLIDER_BOX);
	VOE_TEST_CHECK_FLOAT(shape.half.x, 5.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(shape.half.y, 0.05f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(shape.half.z, 5.0f, 1e-6f);
	VOE_TEST_CHECK(shape.centre.x == 100000.0);
	VOE_TEST_CHECK(shape.centre.y == 0.0 && shape.centre.z == 0.0);
}

// The built-in capsule's collider at scale 1: radius a half, half height one.
// An entity with no collider has no shape.
static void a_capsule_and_no_collider(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_physics_shape shape = { 0 };
	voe_ecs_entity entity = placed(
		world, (voe_math_double3){ 0.0, 0.0, 0.0 },
		(voe_math_float3){ 1.0f, 1.0f, 1.0f },
		(voe_physics_collider){ .kind = VOE_PHYSICS_COLLIDER_CAPSULE,
					.size = { 1.0f, 2.0f, 1.0f } });
	voe_ecs_entity bare = { 0 };

	VOE_TEST_CHECK(voe_physics_shape_of(world, entity, &shape));
	VOE_TEST_CHECK_INT(shape.kind, VOE_PHYSICS_COLLIDER_CAPSULE);
	VOE_TEST_CHECK_FLOAT(shape.half.x, 0.5f, 0.0f);
	VOE_TEST_CHECK_FLOAT(shape.half.y, 1.0f, 0.0f);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &bare));
	VOE_TEST_CHECK(!voe_physics_shape_of(world, bare, &shape));
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);

	registration_tells_a_tool(arena);
	an_intent_is_drained_and_a_bad_size_kept(arena);
	a_scaled_box_far_away(arena);
	a_capsule_and_no_collider(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
