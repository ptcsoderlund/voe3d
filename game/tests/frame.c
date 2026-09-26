// Two game frames draw a world through its own camera on a headless device
// opened with VOE_GAME_CAPACITIES, and what was drawn comes out as a PNG. The
// world is built through the typed creation calls — a camera, a light and one
// cube — not through voe_game_scene_build, which only a cooked scene.c
// defines (game/scene.h).
//
// TWO CASES: that world, and the same world without the light, its light
// table still registered by voe_game_world_new. The second is bug 01 of 024:
// a lightless scene asserted in voe_3d_draw_system_frame; it now draws
// unshaded (0238).
//
// TWO FRAMES each, because the shape system gives a fresh shape its mesh and
// material in the first and the second draws a world that already has them.
// Both at lag 0, the current transforms. Between them a replace intent for the
// cube's collider is submitted, and the second frame's world step drains it.
//
// THE SHADOW CASE: a capsule set over the cube casts onto it, so both are
// drawn into every cascade and the window pass; with VOE_GAME_CAPACITIES every
// pass and object fits and the frame comes back true, lit or not (0258).
//
// THE INTERFACE CASE: a context holding a label, laid out as
// tests/interface.c lays one out, is drawn over the world and the frame comes
// back true: the records fit VOE_GAME_CAPACITIES and draw in the window pass.
//
// Each draw case writes one file into the working directory, checks it is there,
// and removes it, pass or fail. A machine with no usable Vulkan skips and
// says so.
#include <game/frame.h>
#include <game/interface.h>
#include <game/world.h>

#include <app/app.h>

#include <base/arena.h>
#include <base/error.h>

#include <ecs/component.h>

#include <math/quat.h>

#include <physics/collider_system.h>

#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/light_system.h>
#include <scene/transform_system.h>

#include <3d/shape_component.h>

#include <ui/widgets.h>

#include <testing/test.h>

#include <stdbool.h>
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

// A camera back along Z with the default lens, a light when `lit`, and a cube
// at the origin with a collider of 1. Returns the cube.
static voe_ecs_entity build(voe_ecs_world *world, bool lit)
{
	voe_ecs_entity camera, light, cube;
	const voe_scene_camera *lens = voe_ecs_component_default(
		world, voe_ecs_component_type(world, &voe_scene_camera_key));

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &camera));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &cube));

	VOE_TEST_CHECK(
		voe_scene_transform_add(world, camera, placed(0.0f, 0.0f, 5.0f)));
	VOE_TEST_CHECK(voe_scene_camera_add(world, camera, *lens));
	if (lit) {
		VOE_TEST_CHECK(voe_ecs_entity_create(world, &light));
		VOE_TEST_CHECK(voe_scene_light_add(
			world, light,
			(voe_scene_light){ .direction = { -0.4f, -1.0f, -0.6f },
					   .colour = { 1.0f, 1.0f, 1.0f },
					   .intensity = 3.0f }));
	}
	VOE_TEST_CHECK(
		voe_scene_transform_add(world, cube, placed(0.0f, 0.0f, 0.0f)));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, cube,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = VOE_3D_SHAPE_GREY }));
	VOE_TEST_CHECK(voe_physics_collider_add(
		world, cube,
		(voe_physics_collider){ .kind = VOE_PHYSICS_COLLIDER_BOX,
					.size = { 1.0f, 1.0f, 1.0f } }));
	return cube;
}

// A fresh world built `lit` or not, two frames of it, captured, checked on
// disk and removed.
static void draw_case(voe_app *app, voe_base_arena *arena,
		      voe_base_arena *scratch, const voe_3d_shapes *shapes,
		      bool lit)
{
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_base_error error = VOE_BASE_OK;
	voe_ecs_world *world = voe_game_world_new(arena);
	voe_ecs_entity cube = build(world, lit);
	const voe_physics_collider *collider;
	FILE *file;

	VOE_TEST_CHECK(voe_game_frame(app, world, shapes, scratch, size, 0.0f, NULL));
	VOE_TEST_CHECK(voe_physics_collider_submit(
		world, (voe_physics_collider_intent){
			       .entity = cube,
			       .collider = { .kind = VOE_PHYSICS_COLLIDER_BOX,
					     .size = { 2.0f, 1.0f, 1.0f } } }));
	VOE_TEST_CHECK(voe_game_frame(app, world, shapes, scratch, size, 0.0f, NULL));
	collider = voe_physics_collider_get(world, cube);
	VOE_TEST_CHECK(collider != NULL && collider->size.x == 2.0f);

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
	remove(CAPTURE_PATH);
}

// A fresh world built `lit` or not with a capsule two metres over the cube,
// two frames of it, both true: the shadow passes and their casters fit.
static void shadow_case(voe_app *app, voe_base_arena *arena,
			voe_base_arena *scratch, const voe_3d_shapes *shapes,
			bool lit)
{
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_ecs_world *world = voe_game_world_new(arena);
	voe_ecs_entity capsule;

	(void)build(world, lit);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &capsule));
	VOE_TEST_CHECK(voe_scene_transform_add(world, capsule,
					       placed(0.0f, 2.0f, 0.0f)));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, capsule,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CAPSULE,
				.colour = VOE_3D_SHAPE_GREY }));
	VOE_TEST_CHECK(voe_game_frame(app, world, shapes, scratch, size, 0.0f, NULL));
	VOE_TEST_CHECK(voe_game_frame(app, world, shapes, scratch, size, 0.0f, NULL));
}

// One label; the run goes on.
static bool labelled(const voe_game_project_frame *frame)
{
	voe_ui_column_begin(frame->ui, (voe_ui_container){ 0 });
	voe_ui_label(frame->ui, "Coins");
	voe_ui_end(frame->ui);
	VOE_TEST_CHECK(voe_ui_frame_end(frame->ui));
	return true;
}

// A fresh lit world with a label drawn over it: the frame comes back true.
static void interface_case(voe_app *app, voe_base_arena *arena,
			   voe_base_arena *scratch, const voe_3d_shapes *shapes)
{
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_ecs_world *world = voe_game_world_new(arena);
	voe_game_interface *interface =
		voe_game_interface_new(voe_app_device(app), arena);

	VOE_TEST_CHECK(interface != NULL);
	if (interface == NULL)
		return;
	(void)build(world, true);
	VOE_TEST_CHECK(voe_game_interface_run(interface, scratch, world, NULL,
					      size, labelled));
	VOE_TEST_CHECK(voe_ui_element_count(
			       voe_game_interface_context(interface)) > 0);
	VOE_TEST_CHECK(voe_game_frame(app, world, shapes, scratch, size, 0.0f,
				      voe_game_interface_context(interface)));
	voe_game_interface_destroy(interface);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1 << 23);
	voe_base_arena *scratch = voe_base_arena_new(1 << 20);
	voe_app_settings settings = { .width = WIDTH,
				      .height = HEIGHT,
				      .capacities = VOE_GAME_CAPACITIES,
				      .longest_step = 0.25 };
	voe_base_error error = VOE_BASE_OK;
	voe_3d_shapes shapes;
	voe_app *app;

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

	VOE_TEST_CHECK(voe_3d_shapes_upload(voe_app_device(app), &shapes,
					    &error));

	draw_case(app, arena, scratch, &shapes, true);
	draw_case(app, arena, scratch, &shapes, false);
	shadow_case(app, arena, scratch, &shapes, true);
	shadow_case(app, arena, scratch, &shapes, false);
	interface_case(app, arena, scratch, &shapes);

	voe_app_destroy(app);
	voe_base_arena_destroy(scratch);
	voe_base_arena_destroy(arena);
	remove(CAPTURE_PATH);
	return voe_test_result();
}
