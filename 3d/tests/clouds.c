// A frame carries the world's clouds (0492 point 3), through
// voe_3d_draw_system_clouds and voe_3d_draw_system_frame. Needs no graphics
// card: neither takes a device. Its worlds hold a camera, a light and the
// identity and parent tables, the Clouds table only when asked; an entity is
// switched off as 3d/tests/sky_switched_off.c switches one.
//
// It claims six things.
// 1. No Clouds table, and the table with no row, give the zeroed record.
// 2. A Flat row gives VOE_RENDER_CLOUDS_FLAT with its coverage, thickness and
//    shadow strength; a Volumetric one VOE_RENDER_CLOUDS_VOLUMETRIC.
// 3. An eye at y 200 under a height of 1500 gives `bottom` 1300.
// 4. An eye at x 8200 with no drift gives origin x 8; a drift of x 10, the
//    wind blowing towards +X at 10 m/s for a second, at eye x 5 gives 8187.
// 5. The first row off gives the second row, on; with both off, zeroed.
// 6. _frame carries the record the call gives with its eye, and the pass
//    camera carries the same.
#include <3d/draw_system.h>
#include <base/arena.h>
#include <ecs/world.h>
#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/clouds_component.h>
#include <scene/clouds_system.h>
#include <scene/identity_component.h>
#include <scene/identity_system.h>
#include <scene/light_system.h>
#include <scene/parent_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#define SCRATCH (1024 * 1024)
#define ROOM 8

static const voe_scene_camera LENS = { .fov_y = 1.0471976f,
				       .near_plane = 0.1f,
				       .far_plane = 100.0f };

static const voe_scene_clouds FLAT = { .kind = VOE_SCENE_CLOUDS_FLAT,
				       .coverage = 0.3f,
				       .height = 1500.0f,
				       .thickness = 250.0f,
				       .wind_speed = 0.0f,
				       .shadow_strength = 0.4f };

static const voe_scene_clouds VOLUMETRIC = {
	.kind = VOE_SCENE_CLOUDS_VOLUMETRIC,
	.coverage = 0.7f,
	.height = 900.0f,
	.thickness = 600.0f,
	.shadow_strength = 0.8f
};

// A world with a camera standing at `eye` and an empty light table, and the
// Clouds table, room for two, when `clouds`.
static voe_ecs_world *a_world(voe_base_arena *arena, voe_math_double3 eye,
			      bool clouds)
{
	voe_ecs_world *world = voe_ecs_world_new(
		arena, (voe_ecs_limits){ .entities = ROOM,
					 .component_types = 16,
					 .intent_types = 16,
					 .structure_requests = 16,
					 .structure_bytes = 1024 });
	voe_ecs_entity camera = { 0 };

	voe_scene_transform_register(world, ROOM);
	voe_scene_identity_register(world, ROOM);
	voe_scene_parent_register(world, ROOM);
	voe_scene_camera_register(world, 1);
	voe_scene_light_register(world, 1);
	if (clouds)
		voe_scene_clouds_register(world, 2);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &camera));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, camera,
		(voe_scene_transform){ .position = eye,
				       .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_camera_add(world, camera, LENS));
	return world;
}

// A new entity with identity `id`, on, wearing `row`.
static voe_ecs_entity a_row(voe_ecs_world *world, uint64_t id,
			    voe_scene_clouds row)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_identity_add(world, entity,
					      (voe_scene_identity){ .id = id }));
	VOE_TEST_CHECK(voe_scene_clouds_add(world, entity, row));
	return entity;
}

// The entity's switch set to `off` through the identity intent.
static void switch_to(voe_ecs_world *world, voe_ecs_entity entity, bool off)
{
	const voe_scene_identity *read = voe_scene_identity_get(world, entity);
	voe_scene_identity row;

	VOE_TEST_CHECK(read != NULL);
	if (read == NULL)
		return;
	row = *read;
	row.off = off;
	VOE_TEST_CHECK(voe_scene_identity_submit(
		world, (voe_scene_identity_intent){ .entity = entity,
						    .identity = row }));
	voe_scene_identity_system_run(world);
}

static void check_zeroed(voe_render_clouds clouds)
{
	VOE_TEST_CHECK_INT(clouds.kind, VOE_RENDER_CLOUDS_NONE);
	VOE_TEST_CHECK_FLOAT(clouds.coverage, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(clouds.bottom, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(clouds.thickness, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(clouds.origin.x, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(clouds.origin.y, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(clouds.shadow_strength, 0.0f, 0.0f);
}

// The record's kind and the fields copied as authored against `row`.
static void check_row(voe_render_clouds clouds, uint32_t kind,
		      voe_scene_clouds row)
{
	VOE_TEST_CHECK_INT(clouds.kind, kind);
	VOE_TEST_CHECK_FLOAT(clouds.coverage, row.coverage, 0.0f);
	VOE_TEST_CHECK_FLOAT(clouds.thickness, row.thickness, 0.0f);
	VOE_TEST_CHECK_FLOAT(clouds.shadow_strength, row.shadow_strength, 0.0f);
}

static void none_is_zeroed(voe_base_arena *arena)
{
	voe_math_double3 eye = { 0 };

	check_zeroed(voe_3d_draw_system_clouds(a_world(arena, eye, false), eye));
	check_zeroed(voe_3d_draw_system_clouds(a_world(arena, eye, true), eye));
}

static void each_kind_maps_to_render_s(voe_base_arena *arena)
{
	voe_math_double3 eye = { 0 };
	voe_ecs_world *flat = a_world(arena, eye, true);
	voe_ecs_world *deep = a_world(arena, eye, true);

	(void)a_row(flat, 1, FLAT);
	(void)a_row(deep, 1, VOLUMETRIC);
	check_row(voe_3d_draw_system_clouds(flat, eye), VOE_RENDER_CLOUDS_FLAT,
		  FLAT);
	check_row(voe_3d_draw_system_clouds(deep, eye),
		  VOE_RENDER_CLOUDS_VOLUMETRIC, VOLUMETRIC);
}

static void the_bottom_is_above_the_eye(voe_base_arena *arena)
{
	voe_math_double3 eye = { 0.0, 200.0, 0.0 };
	voe_ecs_world *world = a_world(arena, eye, true);

	(void)a_row(world, 1, FLAT);
	VOE_TEST_CHECK_FLOAT(voe_3d_draw_system_clouds(world, eye).bottom,
			     1300.0f, 0.0f);
}

static void the_origin_wraps_less_the_drift(voe_base_arena *arena)
{
	voe_math_double3 far = { 8200.0, 0.0, 0.0 };
	voe_math_double3 near = { 5.0, 0.0, 0.0 };
	voe_ecs_world *still = a_world(arena, far, true);
	voe_ecs_world *blown = a_world(arena, near, true);
	voe_scene_clouds windy = FLAT;
	voe_render_clouds clouds;

	(void)a_row(still, 1, FLAT);
	clouds = voe_3d_draw_system_clouds(still, far);
	VOE_TEST_CHECK_FLOAT(clouds.origin.x, 8.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(clouds.origin.y, 0.0f, 0.0f);

	windy.wind_direction = 90.0f;
	windy.wind_speed = 10.0f;
	(void)a_row(blown, 1, windy);
	voe_scene_clouds_system_run(blown, 1.0f);
	clouds = voe_3d_draw_system_clouds(blown, near);
	VOE_TEST_CHECK_FLOAT(clouds.origin.x, 8187.0f, 1e-3f);
	VOE_TEST_CHECK(clouds.origin.y >= 0.0f &&
		       clouds.origin.y < VOE_RENDER_CLOUDS_PERIOD);
	VOE_TEST_CHECK(clouds.origin.y < 1e-3f ||
		       clouds.origin.y > VOE_RENDER_CLOUDS_PERIOD - 1e-3f);
}

static void an_off_row_gives_the_next_on(voe_base_arena *arena)
{
	voe_math_double3 eye = { 0 };
	voe_ecs_world *world = a_world(arena, eye, true);
	voe_ecs_entity first = a_row(world, 1, FLAT);
	voe_ecs_entity second = a_row(world, 2, VOLUMETRIC);

	switch_to(world, first, true);
	check_row(voe_3d_draw_system_clouds(world, eye),
		  VOE_RENDER_CLOUDS_VOLUMETRIC, VOLUMETRIC);
	switch_to(world, second, true);
	check_zeroed(voe_3d_draw_system_clouds(world, eye));
}

static void the_frame_carries_the_clouds(voe_base_arena *arena)
{
	voe_math_double3 eye = { 8200.0, 200.0, -30.0 };
	voe_ecs_world *world = a_world(arena, eye, true);
	voe_render_clouds want;
	voe_3d_frame frame;
	voe_render_pass_camera camera;

	(void)a_row(world, 1, VOLUMETRIC);
	want = voe_3d_draw_system_clouds(world, eye);
	frame = voe_3d_draw_system_frame(world, (voe_platform_size){ 640, 480 },
					 0.0f);
	camera = voe_3d_draw_system_camera(&frame);
	for (int i = 0; i < 2; i++) {
		voe_render_clouds got = i == 0 ? frame.clouds : camera.clouds;

		VOE_TEST_CHECK_INT(got.kind, want.kind);
		VOE_TEST_CHECK_FLOAT(got.coverage, want.coverage, 0.0f);
		VOE_TEST_CHECK_FLOAT(got.bottom, want.bottom, 0.0f);
		VOE_TEST_CHECK_FLOAT(got.thickness, want.thickness, 0.0f);
		VOE_TEST_CHECK_FLOAT(got.origin.x, want.origin.x, 0.0f);
		VOE_TEST_CHECK_FLOAT(got.origin.y, want.origin.y, 0.0f);
		VOE_TEST_CHECK_FLOAT(got.shadow_strength, want.shadow_strength,
				     0.0f);
	}
	VOE_TEST_CHECK_FLOAT(want.bottom, 700.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(want.origin.y, 8162.0f, 0.0f);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);

	none_is_zeroed(arena);
	each_kind_maps_to_render_s(arena);
	the_bottom_is_above_the_eye(arena);
	the_origin_wraps_less_the_drift(arena);
	an_off_row_gives_the_next_on(arena);
	the_frame_carries_the_clouds(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
