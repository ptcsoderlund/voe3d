// A world's fog volumes become a pass's records (0493 points 1 to 3), through
// voe_3d_fog_volume_shape and voe_3d_fog_volumes. Needs no graphics card:
// neither takes a device. Its worlds hold the transform, parent, identity and
// fog volume tables; an entity is switched off as 3d/tests/clouds.c switches
// one.
//
// It claims eight things.
// 1. No fog volume table, and the table with no row, give the zeroed record.
// 2. A row at (10, 2, −30), size (20, 4, 20), scale 1, seen from the origin is
//    one record whose rows hold the centre at unit place 0 and the point 10 m
//    along +X from it at unit x 1; scale 2 holds that at 20 m.
// 3. Thickness 0.5 gives density 0.025; patchiness and godrays as authored.
// 4. A drift of x 10 at eye x 5 gives patches x 123 and change x 22.
// 5. Eye x 300 with no drift gives patches x 44.
// 6. An off row and a thickness-0 row are left out.
// 7. Nine rows give eight records.
// 8. The shape's half is |scale| × size / 2; no transform is no shape.
#include <3d/fog_volume.h>
#include <base/arena.h>
#include <ecs/world.h>
#include <physics/collider_component.h>
#include <scene/fog_volume_component.h>
#include <scene/fog_volume_system.h>
#include <scene/identity_component.h>
#include <scene/identity_system.h>
#include <scene/parent_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <string.h>

#define SCRATCH (1024 * 1024)
#define ROOM 16

static const voe_scene_fog_volume FOG = { .size = { 20.0f, 4.0f, 20.0f },
					  .thickness = 0.5f,
					  .patchiness = 0.6f,
					  .wind_direction = 0.0f,
					  .wind_speed = 0.0f,
					  .godrays_strength = 0.7f };

static const voe_math_double3 ORIGIN = { 0 };

// A world with the transform, parent and identity tables, and the fog volume
// table when `fog`.
static voe_ecs_world *a_world(voe_base_arena *arena, bool fog)
{
	voe_ecs_world *world = voe_ecs_world_new(
		arena, (voe_ecs_limits){ .entities = ROOM,
					 .component_types = 16,
					 .intent_types = 16,
					 .structure_requests = 16,
					 .structure_bytes = 1024 });

	voe_scene_transform_register(world, ROOM);
	voe_scene_identity_register(world, ROOM);
	voe_scene_parent_register(world, ROOM);
	if (fog)
		voe_scene_fog_volume_register(world, ROOM);
	return world;
}

// A new on entity wearing `row`, at `at` scaled by `scale` unless `placed` is
// false.
static voe_ecs_entity a_row(voe_ecs_world *world, voe_scene_fog_volume row,
			    bool placed, voe_math_double3 at,
			    voe_math_float3 scale)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_identity_add(
		world, entity, (voe_scene_identity){ .id = entity.index + 1 }));
	if (placed)
		VOE_TEST_CHECK(voe_scene_transform_add(
			world, entity,
			(voe_scene_transform){ .position = at,
					       .rotation = { 0.0f, 0.0f, 0.0f,
							     1.0f },
					       .scale = scale }));
	VOE_TEST_CHECK(voe_scene_fog_volume_add(world, entity, row));
	return entity;
}

static voe_ecs_entity a_placed_row(voe_ecs_world *world,
				   voe_scene_fog_volume row,
				   voe_math_double3 at)
{
	return a_row(world, row, true, at, (voe_math_float3){ 1.0f, 1.0f, 1.0f });
}

// The entity's switch set to off through the identity intent.
static void switch_off(voe_ecs_world *world, voe_ecs_entity entity)
{
	const voe_scene_identity *read = voe_scene_identity_get(world, entity);
	voe_scene_identity row;

	VOE_TEST_CHECK(read != NULL);
	if (read == NULL)
		return;
	row = *read;
	row.off = true;
	VOE_TEST_CHECK(voe_scene_identity_submit(
		world, (voe_scene_identity_intent){ .entity = entity,
						    .identity = row }));
	voe_scene_identity_system_run(world);
}

// `p` in the record's unit space along row `axis`.
static float unit_at(voe_render_fog_volume record, int axis, voe_math_float3 p)
{
	voe_math_float4 row = record.rows[axis];

	return row.x * p.x + row.y * p.y + row.z * p.z + row.w;
}

static void none_is_zeroed(voe_base_arena *arena)
{
	voe_render_fog bare = voe_3d_fog_volumes(a_world(arena, false), ORIGIN, 0.0f);
	voe_render_fog empty = voe_3d_fog_volumes(a_world(arena, true), ORIGIN, 0.0f);
	const voe_render_fog zero = { 0 };

	VOE_TEST_CHECK_INT(bare.count, 0);
	VOE_TEST_CHECK_INT(empty.count, 0);
	VOE_TEST_CHECK(memcmp(&bare, &zero, sizeof(zero)) == 0);
	VOE_TEST_CHECK(memcmp(&empty, &zero, sizeof(zero)) == 0);
}

static void the_rows_hold_the_box_about_the_eye(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena, true);
	voe_ecs_world *doubled = a_world(arena, true);
	voe_math_double3 at = { 10.0, 2.0, -30.0 };
	voe_math_float3 centre = { 10.0f, 2.0f, -30.0f };
	voe_render_fog fog;

	(void)a_placed_row(world, FOG, at);
	fog = voe_3d_fog_volumes(world, ORIGIN, 0.0f);
	VOE_TEST_CHECK_INT(fog.count, 1);
	for (int axis = 0; axis < 3; axis++)
		VOE_TEST_CHECK_FLOAT(unit_at(fog.volumes[0], axis, centre), 0.0f,
				     1e-5f);
	VOE_TEST_CHECK_FLOAT(unit_at(fog.volumes[0], 0,
				     (voe_math_float3){ 20.0f, 2.0f, -30.0f }),
			     1.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(fog.volumes[0].density, 0.025f, 1e-7f);
	VOE_TEST_CHECK_FLOAT(fog.volumes[0].patchiness, FOG.patchiness, 0.0f);
	VOE_TEST_CHECK_FLOAT(fog.volumes[0].godrays, FOG.godrays_strength, 0.0f);

	(void)a_row(doubled, FOG, true, at, (voe_math_float3){ 2.0f, 2.0f, 2.0f });
	fog = voe_3d_fog_volumes(doubled, ORIGIN, 0.0f);
	VOE_TEST_CHECK_INT(fog.count, 1);
	VOE_TEST_CHECK_FLOAT(unit_at(fog.volumes[0], 0,
				     (voe_math_float3){ 30.0f, 2.0f, -30.0f }),
			     1.0f, 1e-5f);
}

static void the_fields_wrap_less_the_drift(voe_base_arena *arena)
{
	voe_ecs_world *blown = a_world(arena, true);
	voe_ecs_world *still = a_world(arena, true);
	voe_scene_fog_volume windy = FOG;
	voe_render_fog fog;

	windy.wind_direction = 90.0f;
	windy.wind_speed = 10.0f;
	(void)a_placed_row(blown, windy, ORIGIN);
	voe_scene_fog_volume_system_run(blown, 1.0f);
	fog = voe_3d_fog_volumes(blown, (voe_math_double3){ 5.0, 0.0, 0.0 }, 0.0f);
	VOE_TEST_CHECK_INT(fog.count, 1);
	VOE_TEST_CHECK_FLOAT(fog.volumes[0].patches.x, 123.0f, 1e-4f);
	VOE_TEST_CHECK_FLOAT(fog.volumes[0].change.x, 22.0f, 1e-4f);
	VOE_TEST_CHECK_FLOAT(fog.volumes[0].change.y, 29.5f, 1e-4f);

	(void)a_placed_row(still, FOG, ORIGIN);
	fog = voe_3d_fog_volumes(still, (voe_math_double3){ 300.0, 0.0, 0.0 },
				 0.0f);
	VOE_TEST_CHECK_INT(fog.count, 1);
	VOE_TEST_CHECK_FLOAT(fog.volumes[0].patches.x, 44.0f, 1e-4f);
	VOE_TEST_CHECK_FLOAT(fog.volumes[0].change.x, 12.0f, 1e-4f);
}

// An off row, a thickness-0 row, an unplaced row and a flat row, then one
// kept: only the last is a record.
static void the_left_out_are_left_out(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena, true);
	voe_scene_fog_volume thin = FOG;
	voe_scene_fog_volume flat = FOG;
	voe_math_double3 far = { 0.0, 0.0, -100.0 };
	voe_render_fog fog;

	thin.thickness = 0.0f;
	flat.size.y = 0.0f;
	switch_off(world, a_placed_row(world, FOG, ORIGIN));
	(void)a_placed_row(world, thin, ORIGIN);
	(void)a_row(world, FOG, false, ORIGIN, (voe_math_float3){ 1, 1, 1 });
	(void)a_placed_row(world, flat, ORIGIN);
	(void)a_placed_row(world, FOG, far);
	fog = voe_3d_fog_volumes(world, ORIGIN, 0.0f);
	VOE_TEST_CHECK_INT(fog.count, 1);
	VOE_TEST_CHECK_FLOAT(unit_at(fog.volumes[0], 2,
				     (voe_math_float3){ 0.0f, 0.0f, -100.0f }),
			     0.0f, 1e-5f);
}

static void nine_rows_give_eight(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena, true);

	for (int i = 0; i < 9; i++)
		(void)a_placed_row(world, FOG, (voe_math_double3){ 30.0 * i, 0, 0 });
	VOE_TEST_CHECK_INT(voe_3d_fog_volumes(world, ORIGIN, 0.0f).count,
			   VOE_RENDER_FOG_VOLUMES);
}

static void the_shape_is_the_scaled_size(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena, true);
	voe_ecs_entity box = a_row(world, FOG, true,
				   (voe_math_double3){ 10.0, 2.0, -30.0 },
				   (voe_math_float3){ -2.0f, 0.5f, 1.0f });
	voe_ecs_entity loose =
		a_row(world, FOG, false, ORIGIN, (voe_math_float3){ 1, 1, 1 });
	voe_physics_shape shape = { 0 };

	VOE_TEST_CHECK(voe_3d_fog_volume_shape(world, box, 0.0f, &shape));
	VOE_TEST_CHECK_INT(shape.kind, VOE_PHYSICS_COLLIDER_BOX);
	VOE_TEST_CHECK(shape.centre.x == 10.0 && shape.centre.y == 2.0 &&
		       shape.centre.z == -30.0);
	VOE_TEST_CHECK_FLOAT(shape.half.x, 20.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(shape.half.y, 1.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(shape.half.z, 10.0f, 1e-6f);

	shape.half.x = 99.0f;
	VOE_TEST_CHECK(!voe_3d_fog_volume_shape(world, loose, 0.0f, &shape));
	VOE_TEST_CHECK_FLOAT(shape.half.x, 99.0f, 0.0f);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);

	none_is_zeroed(arena);
	the_rows_hold_the_box_about_the_eye(arena);
	the_fields_wrap_less_the_drift(arena);
	the_left_out_are_left_out(arena);
	nine_rows_give_eight(arena);
	the_shape_is_the_scaled_size(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
