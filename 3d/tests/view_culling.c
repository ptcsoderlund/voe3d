// What the camera cannot see is skipped and counted (ADR-0464 points 2 and 5):
// a camera at the origin looking down −Z on a headless device, as
// 3d/tests/draw_system.c builds one, read through the counts _run returns.
//
// 1. Three red cubes, 5 m ahead, 5 m behind and 50 m to the left: drawn 1,
//    skipped 2, and the centre pixel is red, the front cube's colour.
// 2. One cube whose centre is on the right edge of the picture 5 m ahead:
//    drawn 1, skipped 0, and the pixel just inside that edge is red. A sphere
//    reaching across a plane is kept, so nothing pops at an edge.
// 3. Two model rows wearing a sphere from model_glb.inc, loaded as
//    3d/tests/draw_levels.c loads one, 5 m ahead and 5 m behind: drawn 1,
//    skipped 1.
//
// IT NEEDS A GRAPHICS CARD AND SKIPS WITH A REASON WITHOUT ONE, as
// 3d/tests/draw_system.c does.
#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/models.h>
#include <3d/panel_component.h>
#include <3d/shape_component.h>
#include <3d/shape_system.h>
#include <base/arena.h>
#include <base/error.h>
#include <ecs/world.h>
#include <render/device.h>
#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/light_component.h>
#include <scene/light_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "model_glb.inc"

#define SCRATCH (16 * 1024 * 1024)
#define SIDE 32
#define BALL "Assets/ball.glb"

// The built-in shapes, one small sphere and room to spare; a few objects and
// the one pass.
static const voe_render_capacities CAPACITIES = {
	.vertices = VOE_3D_SHAPES_VERTICES + 1024,
	.indices = VOE_3D_SHAPES_INDICES + 4096,
	.geometries = VOE_3D_SHAPES_GEOMETRIES + 4,
	.objects = 8,
	.shadings = VOE_3D_SHAPES_SHADINGS + 8,
	.passes = 1,
};

// 60° tall: at 5 m the picture's edge is 5 tan 30° from its middle.
static const voe_scene_camera LENS = { .fov_y = 1.0471976f,
				       .near_plane = 0.1f,
				       .far_plane = 100.0f };

static voe_scene_transform at(double x, double z)
{
	return (voe_scene_transform){ .position = { x, 0.0, z },
				      .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				      .scale = { 1.0f, 1.0f, 1.0f } };
}

// The camera at the origin looking down −Z and a sun shining down and away
// from it, so a face toward the camera is lit.
static voe_ecs_world *a_world(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = 8,
		.component_types = 16,
		.intent_types = 16,
		.structure_requests = 16,
		.structure_bytes = 1024,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);
	voe_ecs_entity entity = { 0 };

	voe_scene_transform_register(world, 8);
	voe_scene_camera_register(world, 1);
	voe_scene_light_register(world, 1);
	voe_3d_mesh_register(world, 8);
	voe_3d_material_register(world, 8);
	voe_3d_panel_register(world, 1);
	voe_3d_model_register(world, 4);
	voe_3d_shape_register(world, 4);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, at(0.0, 0.0)));
	VOE_TEST_CHECK(voe_scene_camera_add(world, entity, LENS));

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){
			.rotation = voe_scene_light_facing(
				(voe_math_float3){ 0.0f, -0.6f, -0.8f }),
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_light_add(
		world, entity,
		(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 3.0f }));
	return world;
}

static void add_a_cube(voe_ecs_world *world, double x, double z)
{
	voe_ecs_entity cube = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &cube));
	VOE_TEST_CHECK(voe_scene_transform_add(world, cube, at(x, z)));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, cube,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = { 1.0f, 0.0f, 0.0f } }));
}

static void add_a_ball(voe_ecs_world *world, double z)
{
	voe_ecs_entity ball = { 0 };
	voe_3d_model row = { .fade = 0.0f };

	snprintf(row.path, sizeof(row.path), "%s", BALL);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &ball));
	VOE_TEST_CHECK(voe_scene_transform_add(world, ball, at(0.0, z)));
	VOE_TEST_CHECK(voe_3d_model_add(world, ball, row));
}

// One frame of `world` drawn from `models`: what _run counted.
static voe_3d_view_counts one_frame(voe_ecs_world *world,
				    voe_render_device *device,
				    voe_base_arena *arena,
				    const voe_3d_models *models)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_3d_view_counts counts = { 0 };
	voe_render_pass_camera camera;
	bool drawing = false;

	frame.models = models;
	camera = voe_3d_draw_system_camera(&frame);
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return counts;
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	counts = voe_3d_draw_system_run(world, device, arena, frame);
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	return counts;
}

// Whether the last frame's middle row is red at its centre, or with
// `right_edge` at its last pixel: red high, green and blue under half of it, a
// lit dielectric's highlight aside.
static bool red_at(voe_render_device *device, voe_base_arena *arena,
		   bool right_edge)
{
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	const uint8_t *pixel;
	uint32_t x;

	VOE_TEST_CHECK(voe_render_target_read(device, VOE_RENDER_TARGET_WINDOW,
					      arena, &picture, &error));
	if (picture.pixels == NULL)
		return false;
	x = right_edge ? picture.width - 1 : picture.width / 2;
	pixel = picture.pixels +
		((size_t)(picture.height / 2) * picture.width + x) * 4;
	return pixel[0] > 128 && pixel[1] < pixel[0] / 2 &&
	       pixel[2] < pixel[0] / 2;
}

static void only_the_cube_ahead_draws(voe_base_arena *arena,
				      voe_render_device *device,
				      const voe_3d_shapes *shapes)
{
	voe_ecs_world *world = a_world(arena);
	voe_3d_view_counts counts;

	add_a_cube(world, 0.0, -5.0);
	add_a_cube(world, 0.0, 5.0);
	add_a_cube(world, -50.0, 0.0);
	voe_3d_shape_system_run(world, shapes);
	counts = one_frame(world, device, arena, NULL);
	VOE_TEST_CHECK_INT(counts.drawn, 1);
	VOE_TEST_CHECK_INT(counts.skipped, 2);
	VOE_TEST_CHECK(red_at(device, arena, false));
}

static void a_cube_on_the_edge_draws(voe_base_arena *arena,
				     voe_render_device *device,
				     const voe_3d_shapes *shapes)
{
	voe_ecs_world *world = a_world(arena);
	voe_3d_view_counts counts;

	add_a_cube(world, 5.0 * tan(0.5 * LENS.fov_y), -5.0);
	voe_3d_shape_system_run(world, shapes);
	counts = one_frame(world, device, arena, NULL);
	VOE_TEST_CHECK_INT(counts.drawn, 1);
	VOE_TEST_CHECK_INT(counts.skipped, 0);
	VOE_TEST_CHECK(red_at(device, arena, true));
}

static void a_model_behind_is_skipped(voe_base_arena *arena,
				      voe_render_device *device,
				      const voe_3d_shapes *shapes)
{
	voe_ecs_world *world = a_world(arena);
	voe_3d_models *models = voe_3d_models_new();
	voe_base_error error = VOE_BASE_OK;
	size_t size = 0;
	const uint8_t *glb = sphere_glb(arena, 8, 16, false, &size);
	voe_3d_view_counts counts;

	VOE_TEST_CHECK(voe_3d_models_load_levelled(models, device, BALL, 1, glb,
						   size, NULL, 0, &error));
	add_a_ball(world, -5.0);
	add_a_ball(world, 5.0);
	voe_3d_shape_system_run(world, shapes);
	counts = one_frame(world, device, arena, models);
	VOE_TEST_CHECK_INT(counts.drawn, 1);
	VOE_TEST_CHECK_INT(counts.skipped, 1);
	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, CAPACITIES, &error);
	voe_3d_shapes shapes;

	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(device != NULL);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}
	VOE_TEST_CHECK(voe_3d_shapes_upload(device, &shapes, &error));

	only_the_cube_ahead_draws(arena, device, &shapes);
	a_cube_on_the_edge_draws(arena, device, &shapes);
	a_model_behind_is_skipped(arena, device, &shapes);

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
