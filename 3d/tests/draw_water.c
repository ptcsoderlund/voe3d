// Water is drawn (ADR-0305 point 7): a camera four metres up looks straight
// down on a 4 × 4 water at y = 0, over a grey ground cube whose top is at
// y = -0.95, under a slanted sun, with the store's water record loaded.
//
// THE CENTRE CHANGES. At four metres a sixty-degree field spans 2.3 m either
// way, so the picture's centre is water: it differs from the same frame of a
// world with no water in it.
//
// THE CLOCK MOVES THE WAVES. The same world drawn with its clock at 0 and at
// 1.3 s gives two different pictures, read whole: waves bend the normals only,
// so what changes is the shading, not where the plane is.
//
// A STORE WITHOUT THE RECORD DRAWS NO WATER and fails nothing: the frame issues
// as many draws as the world with no water, and its centre is the same.
//
// WATER CASTS NO SHADOW: the shadow passes issue as many draws with the water
// as without it.
//
// IT NEEDS A GRAPHICS CARD AND SKIPS WITH A REASON WITHOUT ONE, as
// 3d/tests/draw_system.c does.
#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/models.h>
#include <3d/panel_component.h>
#include <3d/shadow_cascades.h>
#include <3d/shape_component.h>
#include <3d/shape_system.h>
#include <3d/water_component.h>
#include <3d/water_system.h>
#include <base/arena.h>
#include <base/error.h>
#include <ecs/world.h>
#include <math/float3.h>
#include <render/device.h>
#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/light_component.h>
#include <scene/light_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <stdio.h>
#include <string.h>

#define SCRATCH (4 * 1024 * 1024)
#define SIDE 32

// The shapes, the store's quad and water record; the ground and the water, each
// drawn once more into each of the four cascades and the bounce map at most;
// four shadow passes and the bounce pass before the view's one.
static const voe_render_capacities CAPACITIES = {
	.vertices = VOE_3D_SHAPES_VERTICES + 4,
	.indices = VOE_3D_SHAPES_INDICES + 6,
	.geometries = VOE_3D_SHAPES_GEOMETRIES + 1,
	.objects = 2 + (VOE_RENDER_SHADOW_CASCADES + 1) * 2,
	.shadings = VOE_3D_SHAPES_SHADINGS + 1,
	.passes = 2 + VOE_RENDER_SHADOW_CASCADES,
	.shadow_size = VOE_3D_SHADOW_TEXELS,
};

// The camera looking down, the sun, the ground and, with `water`, the 4 × 4
// water with its waves row added by one run of the water system.
static voe_ecs_world *a_world(voe_base_arena *arena, const voe_3d_shapes *shapes,
			      bool water)
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
	voe_3d_mesh_register(world, 4);
	voe_3d_material_register(world, 4);
	voe_3d_panel_register(world, 1);
	voe_3d_shape_register(world, 4);
	voe_3d_model_register(world, 1);
	voe_3d_water_register(world, 2);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){
			.position = { 0.0, 4.0, 0.0 },
			.rotation = { -0.70710678f, 0.0f, 0.0f, 0.70710678f },
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
				(voe_math_float3){ 0.4f, -1.0f, 0.3f }),
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_light_add(
		world, entity,
		(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 3.0f,
				   .fill_colour = { 1.0f, 1.0f, 1.0f },
				   .fill_intensity = 0.2f }));

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .position = { 0.0, -1.0, 0.0 },
				       .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 20.0f, 0.1f, 20.0f } }));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, entity,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = { 0.5f, 0.5f, 0.5f } }));
	voe_3d_shape_system_run(world, shapes);

	if (water) {
		VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
		VOE_TEST_CHECK(voe_scene_transform_add(
			world, entity,
			(voe_scene_transform){ .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
					       .scale = { 1.0f, 1.0f, 1.0f } }));
		VOE_TEST_CHECK(voe_3d_water_add(
			world, entity,
			(voe_3d_water){ .width = 4.0f,
					.length = 4.0f,
					.wave_height = 0.05f,
					.wave_length = 2.0f,
					.deep = 3.0f,
					.colour = { 0.02f, 0.12f, 0.15f },
					.sky = { 0.6f, 0.75f, 0.95f } }));
	}
	voe_3d_water_system_run(world, 0.0f);
	return world;
}

// One frame of `world` from `models`, with its shadows: the view pass's draws
// in `drawn`, the shadow passes' in `cast`, and the picture.
static voe_render_picture a_frame(voe_ecs_world *world, voe_render_device *device,
				  voe_base_arena *arena,
				  const voe_3d_models *models, uint32_t *drawn,
				  uint32_t *cast)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_render_pass_camera camera;
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;
	uint32_t before;

	frame.models = models;
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return picture;
	VOE_TEST_CHECK(voe_3d_draw_system_shadows(world, device, &frame));
	*cast = voe_render_frame_draw_count(device);
	camera = (voe_render_pass_camera){ frame.view, frame.light, frame.shadow };
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	before = voe_render_frame_draw_count(device);
	voe_3d_draw_system_run(world, device, arena, frame);
	*drawn = voe_render_frame_draw_count(device) - before;
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	VOE_TEST_CHECK(voe_render_target_read(device, VOE_RENDER_TARGET_WINDOW,
					      arena, &picture, &error));
	VOE_TEST_CHECK(picture.pixels != NULL);
	return picture;
}

// The picture's centre pixel, rgb packed.
static uint32_t centre(voe_render_picture picture)
{
	const uint8_t *pixel;

	if (picture.pixels == NULL)
		return 0;
	pixel = picture.pixels + ((size_t)(picture.height / 2) * picture.width +
				  picture.width / 2) *
					 4;
	printf("centre %u %u %u\n", pixel[0], pixel[1], pixel[2]);
	return (uint32_t)pixel[0] << 16 | (uint32_t)pixel[1] << 8 | pixel[2];
}

// Whether two pictures differ anywhere.
static bool differ(voe_render_picture a, voe_render_picture b)
{
	if (a.pixels == NULL || b.pixels == NULL)
		return false;
	return memcmp(a.pixels, b.pixels, (size_t)a.width * a.height * 4) != 0;
}

static void water_is_drawn(voe_base_arena *arena, voe_render_device *device,
			   const voe_3d_shapes *shapes,
			   const voe_3d_models *models,
			   const voe_3d_models *empty)
{
	voe_ecs_world *dry = a_world(arena, shapes, false);
	voe_ecs_world *wet = a_world(arena, shapes, true);
	uint32_t dry_drawn = 0, dry_cast = 0, wet_drawn = 0, wet_cast = 0;
	uint32_t bare_drawn = 0, bare_cast = 0;
	voe_render_picture none = a_frame(dry, device, arena, models, &dry_drawn,
					  &dry_cast);
	voe_render_picture still = a_frame(wet, device, arena, models,
					   &wet_drawn, &wet_cast);
	voe_render_picture bare = a_frame(wet, device, arena, empty, &bare_drawn,
					  &bare_cast);
	voe_render_picture moved;

	// The centre is water, and the water is one draw more.
	VOE_TEST_CHECK(centre(still) != centre(none));
	VOE_TEST_CHECK_INT(wet_drawn, dry_drawn + 1);
	// No shadow from the water.
	VOE_TEST_CHECK(dry_cast > 0);
	VOE_TEST_CHECK_INT(wet_cast, dry_cast);
	// No record, no water, and nothing else lost.
	VOE_TEST_CHECK_INT(bare_drawn, dry_drawn);
	VOE_TEST_CHECK_INT(bare_cast, dry_cast);
	VOE_TEST_CHECK_INT(centre(bare), centre(none));
	// The clock at 1.3 s moves the waves.
	voe_3d_water_system_run(wet, 1.3f);
	moved = a_frame(wet, device, arena, models, &wet_drawn, &wet_cast);
	VOE_TEST_CHECK(differ(still, moved));
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, CAPACITIES, &error);
	voe_3d_shapes shapes;
	voe_3d_models *models;
	voe_3d_models *empty;

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
	models = voe_3d_models_new();
	empty = voe_3d_models_new();
	VOE_TEST_CHECK(voe_3d_models_load_water(models, device, &error));

	water_is_drawn(arena, device, &shapes, models, empty);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
	voe_3d_models_destroy(empty);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
