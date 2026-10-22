// The shadows call times its parts by a clock it is handed (0461 point 2):
// voe_3d_draw_system_shadows adds each part's seconds to the frame's
// `shadow_times` when the frame has a `clock`, and touches nothing without one.
//
// THE WORLD IS shadows.c's: a camera at the origin looking along -Z, a
// flattened cube a metre below as the floor and a cube a metre above it five
// metres out, under a sun straight down that casts, with bounces 0. So the
// cascades draw two casters and nothing bounces.
//
// UNTIMED: a frame as voe_3d_draw_system_frame gives it, no clock, comes back
// from the call with all five parts at 0.
//
// TIMED: a fake clock that reads one more second each time it is read. Each
// cascade's walk is a read either side, so `casters` is above 0; nothing
// bounces, so `bounce` is never read around and stays 0.
//
// IT NEEDS A GRAPHICS CARD AND SKIPS WITH A REASON WITHOUT ONE, as
// 3d/tests/shadows.c does.
#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/panel_component.h>
#include <3d/shadow_cascades.h>
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

#include <stdio.h>

#define SCRATCH (4 * 1024 * 1024)
#define SIDE 64

// Two casters drawn once into each of the four cascades with one share over,
// and the four shadow passes with one over.
static const voe_render_capacities CAPACITIES = {
	.vertices = VOE_3D_SHAPES_VERTICES,
	.indices = VOE_3D_SHAPES_INDICES,
	.geometries = VOE_3D_SHAPES_GEOMETRIES,
	.objects = 3 + (VOE_RENDER_SHADOW_CASCADES + 1) * 3,
	.shadings = VOE_3D_SHAPES_SHADINGS,
	.passes = 3 + VOE_RENDER_SHADOW_CASCADES,
	.targets = 1,
	.shadow_size = VOE_3D_SHADOW_TEXELS,
	.point_shadow_size = VOE_3D_POINT_SHADOW_TEXELS,
};

// The fake clock's last reading, in seconds.
static double fake_now;

// One more second on each read.
static double fake_clock(void)
{
	fake_now += 1.0;
	return fake_now;
}

// One grey casting cube at (`x`, `y`, `z`), scaled by `scale`.
static void add_a_shape(voe_ecs_world *world, double x, double y, double z,
			voe_math_float3 scale)
{
	voe_ecs_entity entity = { 0 };
	voe_scene_transform transform = {
		.position = { x, y, z },
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = scale,
	};

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, transform));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, entity,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = { 0.5f, 0.5f, 0.5f },
				.cast_shadows = true }));
}

// The camera, the floor, the cube and the casting sun straight down.
static voe_ecs_world *a_world(voe_base_arena *arena, const voe_3d_shapes *shapes)
{
	voe_ecs_limits limits = {
		.entities = 8,
		.component_types = 8,
		.intent_types = 8,
		.structure_requests = 8,
		.structure_bytes = 256,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);
	voe_ecs_entity entity = { 0 };

	voe_scene_transform_register(world, 8);
	voe_scene_camera_register(world, 2);
	voe_scene_light_register(world, 2);
	voe_3d_mesh_register(world, 8);
	voe_3d_material_register(world, 8);
	voe_3d_panel_register(world, 8);
	voe_3d_shape_register(world, 8);
	voe_3d_model_register(world, 8);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_camera_add(
		world, entity,
		(voe_scene_camera){ .fov_y = 1.0471976f,
				    .near_plane = 0.1f,
				    .far_plane = 100.0f }));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){
			.rotation = voe_scene_light_facing(
				(voe_math_float3){ 0.0f, -1.0f, 0.0f }),
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_light_add(
		world, entity,
		(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 3.0f,
				   .bounces = 0,
				   .cast_shadows = true }));
	add_a_shape(world, 0.0, -1.0, -5.0, (voe_math_float3){ 20.0f, 0.1f, 20.0f });
	add_a_shape(world, 0.0, 1.0, -5.0, (voe_math_float3){ 1.0f, 1.0f, 1.0f });
	voe_3d_shape_system_run(world, shapes);
	return world;
}

// The shadow times of one frame of a_world's world through the shadows call,
// with `clock` as its clock; the frame is begun and ended with no view pass.
static voe_3d_shadow_times a_frame(voe_render_device *device,
				   const voe_3d_shapes *shapes,
				   double (*clock)(void))
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_ecs_world *world = a_world(arena, shapes);
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_3d_shadow_times times = { 0 };
	bool drawing = false;

	VOE_TEST_CHECK(frame.clock == NULL);
	frame.clock = clock;
	VOE_TEST_CHECK(voe_3d_draw_system_point_lights(world, &frame, arena));
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (drawing) {
		VOE_TEST_CHECK(voe_3d_draw_system_shadows(world, device, &frame));
		VOE_TEST_CHECK_INT(frame.shadow.count, VOE_RENDER_SHADOW_CASCADES);
		times = frame.shadow_times;
		VOE_TEST_CHECK(voe_render_frame_end(device));
	}
	voe_base_arena_destroy(arena);
	printf("casters %g, ground %g, foliage %g, lamps %g, bounce %g\n",
	       times.casters, times.ground, times.foliage, times.lamps,
	       times.bounce);
	return times;
}

// A frame with no clock leaves all five at 0.
static void shadow_times_untimed(voe_render_device *device,
				 const voe_3d_shapes *shapes)
{
	voe_3d_shadow_times times = a_frame(device, shapes, NULL);

	VOE_TEST_CHECK(times.casters == 0.0);
	VOE_TEST_CHECK(times.ground == 0.0);
	VOE_TEST_CHECK(times.foliage == 0.0);
	VOE_TEST_CHECK(times.lamps == 0.0);
	VOE_TEST_CHECK(times.bounce == 0.0);
}

// With the fake clock the casters took time and the bounce, never called,
// none.
static void shadow_times_casters(voe_render_device *device,
				 const voe_3d_shapes *shapes)
{
	voe_3d_shadow_times times = a_frame(device, shapes, fake_clock);

	VOE_TEST_CHECK(times.casters > 0.0);
	VOE_TEST_CHECK(times.bounce == 0.0);
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

	shadow_times_untimed(device, &shapes);
	shadow_times_casters(device, &shapes);

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
