// Two game frames draw a world through its own camera on a headless device
// opened with VOE_GAME_CAPACITIES, and what was drawn comes out as a PNG. The
// world is built through the typed creation calls — a camera, a light and one
// cube — not through voe_game_scene_build, which only a cooked scene.c
// defines (game/scene.h).
//
// TWO FRAMES, because the shape system gives a fresh shape its mesh and
// material in the first and the second draws a world that already has them.
//
// It writes one file into the working directory, checks it is there, and
// removes it, pass or fail. A machine with no usable Vulkan skips and says so.
#include <game/frame.h>
#include <game/world.h>

#include <app/app.h>

#include <base/arena.h>
#include <base/error.h>

#include <ecs/component.h>

#include <math/quat.h>

#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/light_system.h>
#include <scene/transform_system.h>

#include <3d/shape_component.h>

#include <testing/test.h>

#include <stdio.h>

#define WIDTH 128
#define HEIGHT 72
#define CAPTURE_PATH "game_frame_test.png"

// An unrotated pose at `position`, scale one.
static voe_scene_transform placed(float x, float y, float z)
{
	return (voe_scene_transform){
		.position = { x, y, z },
		.rotation = voe_math_quat_from_axis_angle(
			(voe_math_float3){ 0.0f, 1.0f, 0.0f }, 0.0f),
		.scale = { 1.0f, 1.0f, 1.0f },
	};
}

// A camera back along Z with the default lens, a light and a cube at the
// origin.
static void build(voe_ecs_world *world)
{
	voe_ecs_entity camera, light, cube;
	const voe_scene_camera *lens = voe_ecs_component_default(
		world, voe_ecs_component_type(world, &voe_scene_camera_key));

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &camera));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &light));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &cube));

	VOE_TEST_CHECK(
		voe_scene_transform_add(world, camera, placed(0.0f, 0.0f, 5.0f)));
	VOE_TEST_CHECK(voe_scene_camera_add(world, camera, *lens));
	VOE_TEST_CHECK(voe_scene_light_add(
		world, light,
		(voe_scene_light){ .direction = { -0.4f, -1.0f, -0.6f },
				   .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 3.0f }));
	VOE_TEST_CHECK(
		voe_scene_transform_add(world, cube, placed(0.0f, 0.0f, 0.0f)));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, cube,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = VOE_3D_SHAPE_GREY }));
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1 << 22);
	voe_base_arena *scratch = voe_base_arena_new(1 << 20);
	voe_app_settings settings = { .width = WIDTH,
				      .height = HEIGHT,
				      .capacities = VOE_GAME_CAPACITIES,
				      .longest_step = 0.25 };
	voe_base_error error = VOE_BASE_OK;
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_3d_shapes shapes;
	voe_ecs_world *world;
	voe_app *app;
	FILE *file;

	app = voe_app_new_headless(arena, scratch, settings, &error);
	if (app == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(app != NULL);
		voe_base_arena_destroy(scratch);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}
	voe_base_arena_clear(scratch);

	world = voe_game_world_new(arena);
	build(world);
	VOE_TEST_CHECK(voe_3d_shapes_upload(voe_app_device(app), &shapes,
					    &error));

	VOE_TEST_CHECK(voe_game_frame(app, world, &shapes, scratch, size));
	VOE_TEST_CHECK(voe_game_frame(app, world, &shapes, scratch, size));

	VOE_TEST_CHECK(voe_app_capture_png(app, VOE_RENDER_TARGET_WINDOW,
					   scratch, CAPTURE_PATH, &error));

	// The MSVC runtime deprecates fopen and -Werror makes that an error;
	// the file is read by something that is not the platform that wrote it.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	file = fopen(CAPTURE_PATH, "rb");
#pragma clang diagnostic pop
	VOE_TEST_CHECK(file != NULL);
	if (file != NULL)
		fclose(file);

	voe_app_destroy(app);
	voe_base_arena_destroy(scratch);
	voe_base_arena_destroy(arena);
	remove(CAPTURE_PATH);
	return voe_test_result();
}
