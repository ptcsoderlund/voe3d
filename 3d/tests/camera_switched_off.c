// A camera that is off frames as the camera table's default row at the
// identity transform (0441 point 3), through voe_3d_draw_system_frame. Needs no
// graphics card: it takes no device. Its world is built as
// 3d/tests/lights_switched_off.c builds its own, plus the parent table; the
// camera's entity has an identity, stands at (3, 2, 6) turned 30 degrees about
// +Y, and has a lens of fov 0.5 and planes 0.5 and 200. Framed at 1280 by 720.
//
// It claims five things.
// 1. On: `eye` is (3, 2, 6) and `view` is voe_3d_view's for that pose and lens.
// 2. Off: `eye` is nought, `blind` false, and `view` is voe_3d_view's for the
//    identity pose and the table's default row.
// 3. On again: as 1.
// 4. Parented under an unmoved entity with an identity, the parent off: as 2;
//    the parent on: as 1.
// 5. Off, framed with lag 0.5: still as 2.
#include <3d/draw_system.h>
#include <3d/projection.h>
#include <base/arena.h>
#include <ecs/component.h>
#include <ecs/structure.h>
#include <ecs/world.h>
#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/identity_component.h>
#include <scene/identity_system.h>
#include <scene/light_system.h>
#include <scene/parent_component.h>
#include <scene/parent_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#define SCRATCH (1024 * 1024)
#define ROOM 8

static const voe_scene_transform UNMOVED = { .rotation = { 0.0f, 0.0f, 0.0f,
							   1.0f },
					     .scale = { 1.0f, 1.0f, 1.0f } };

// (3, 2, 6), turned 30 degrees about +Y: sin and cos of 15 degrees.
static const voe_scene_transform PLACED = {
	.position = { 3.0, 2.0, 6.0 },
	.rotation = { 0.0f, 0.25881904f, 0.0f, 0.96592583f },
	.scale = { 1.0f, 1.0f, 1.0f }
};

static const voe_scene_camera LENS = { .fov_y = 0.5f,
				       .near_plane = 0.5f,
				       .far_plane = 200.0f };

static const voe_platform_size SIZE = { 1280, 720 };

// A world with the camera's entity placed, its identity on, no light, and room
// for a parent.
static voe_ecs_world *world_of(voe_base_arena *arena, voe_ecs_entity *camera)
{
	voe_ecs_limits limits = {
		.entities = ROOM,
		.component_types = 8,
		.intent_types = 8,
		.structure_requests = 16,
		.structure_bytes = 1024,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_transform_register(world, ROOM);
	voe_scene_identity_register(world, ROOM);
	voe_scene_parent_register(world, ROOM);
	voe_scene_camera_register(world, 1);
	voe_scene_light_register(world, 1);
	*camera = (voe_ecs_entity){ 0 };
	VOE_TEST_CHECK(voe_ecs_entity_create(world, camera));
	VOE_TEST_CHECK(voe_scene_transform_add(world, *camera, PLACED));
	VOE_TEST_CHECK(voe_scene_identity_add(world, *camera,
					      (voe_scene_identity){ .id = 1 }));
	VOE_TEST_CHECK(voe_scene_camera_add(world, *camera, LENS));
	return world;
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

static void check_matrix(voe_math_float4x4 got, voe_math_float4x4 want)
{
	for (int row = 0; row < 4; row++)
		for (int column = 0; column < 4; column++)
			VOE_TEST_CHECK_FLOAT(got.m[row][column],
					     want.m[row][column], 1e-5f);
}

// The frame against a camera at `pose` with `lens`, its eye at `pose`'s.
static void check_framed(voe_3d_frame frame, voe_scene_transform pose,
			 voe_scene_camera lens)
{
	voe_render_view want = { 0 };

	VOE_TEST_CHECK(voe_3d_view(pose, lens, 1280.0f / 720.0f, &want));
	VOE_TEST_CHECK(!frame.blind);
	VOE_TEST_CHECK_FLOAT((float)frame.eye.x, (float)pose.position.x, 1e-5f);
	VOE_TEST_CHECK_FLOAT((float)frame.eye.y, (float)pose.position.y, 1e-5f);
	VOE_TEST_CHECK_FLOAT((float)frame.eye.z, (float)pose.position.z, 1e-5f);
	check_matrix(frame.view.view, want.view);
	check_matrix(frame.view.projection, want.projection);
}

static void check_on(voe_ecs_world *world, float lag)
{
	check_framed(voe_3d_draw_system_frame(world, SIZE, lag), PLACED, LENS);
}

static void check_off(voe_ecs_world *world, float lag)
{
	const voe_scene_camera *fallback = voe_ecs_component_default(
		world, voe_ecs_component_type(world, &voe_scene_camera_key));

	VOE_TEST_CHECK(fallback != NULL);
	if (fallback == NULL)
		return;
	check_framed(voe_3d_draw_system_frame(world, SIZE, lag), UNMOVED,
		     *fallback);
}

static void an_off_camera_frames_from_the_origin(voe_base_arena *arena)
{
	voe_ecs_entity camera;
	voe_ecs_world *world = world_of(arena, &camera);

	check_on(world, 0.0f);
	switch_to(world, camera, true);
	check_off(world, 0.0f);
	switch_to(world, camera, false);
	check_on(world, 0.0f);
}

static void an_off_parent_takes_the_camera(voe_base_arena *arena)
{
	voe_ecs_entity camera;
	voe_ecs_world *world = world_of(arena, &camera);
	voe_ecs_entity parent = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &parent));
	VOE_TEST_CHECK(voe_scene_transform_add(world, parent, UNMOVED));
	VOE_TEST_CHECK(voe_scene_identity_add(world, parent,
					      (voe_scene_identity){ .id = 2 }));
	VOE_TEST_CHECK(voe_scene_parent_set(world, camera, parent));
	voe_ecs_structure_apply(world);
	voe_scene_transform_system_run(world);
	switch_to(world, parent, true);
	check_off(world, 0.0f);
	switch_to(world, parent, false);
	check_on(world, 0.0f);
}

static void an_off_camera_takes_no_lag(voe_base_arena *arena)
{
	voe_ecs_entity camera;
	voe_ecs_world *world = world_of(arena, &camera);

	switch_to(world, camera, true);
	check_off(world, 0.5f);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);

	an_off_camera_frames_from_the_origin(arena);
	an_off_parent_takes_the_camera(arena);
	an_off_camera_takes_no_lag(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
