// The draws use the level the camera chooses (ADR-0425 point 5): a camera at
// the origin looks down −Z under a slanted sun that casts, and one model row
// wearing a 64 × 32 unit sphere from model_glb.inc stands on that axis,
// casting too. The frame's triangle count, every pass's, is read after a
// whole frame: the shadows call, then the view's pass and the draw system.
//
// 1. The sphere loaded with its levels file, 3 m before the camera: N.
// 2. The same 400 m away: under N ÷ 4, since there the camera chooses a level
//    far coarser than at 3 m, and the cascades draw what it chooses.
// 3. The sphere loaded with NULL levels, 400 m away: over four times check 2's
//    count. A model without levels is drawn whole at any distance.
//
// Radius 1 on the camera's axis, so both distances are in view; the far plane
// is past 400 m. Nothing bounces, so no capture adds its last level.
//
// IT NEEDS A GRAPHICS CARD AND SKIPS WITH A REASON WITHOUT ONE, as
// 3d/tests/draw_model_fade.c does.
#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/model_levels.h>
#include <3d/models.h>
#include <3d/panel_component.h>
#include <3d/shadow_cascades.h>
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
#define RINGS 32
#define SEGMENTS 64
#define LEVELLED "Assets/ball.glb"
#define PLAIN "Assets/plain.glb"

// Two copies of the sphere, one with up to four levels; the sphere drawn once
// into the view and once into each cascade; four shadow passes and the view's.
static const voe_render_capacities CAPACITIES = {
	.vertices = 32 * 1024,
	.indices = 128 * 1024,
	.geometries = 8,
	.objects = 2 * (1 + VOE_RENDER_SHADOW_CASCADES),
	.shadings = 16,
	.passes = 1 + VOE_RENDER_SHADOW_CASCADES,
	.shadow_size = VOE_3D_SHADOW_TEXELS,
};

// The camera at the origin looking down −Z, the sun, and one model row
// wearing `path` `distance` metres before the camera.
static voe_ecs_world *a_world(voe_base_arena *arena, const char *path,
			      double distance)
{
	voe_ecs_limits limits = {
		.entities = 4,
		.component_types = 16,
		.intent_types = 16,
		.structure_requests = 16,
		.structure_bytes = 1024,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);
	voe_ecs_entity entity = { 0 };
	voe_3d_model row = { .cast_shadows = true };

	voe_scene_transform_register(world, 4);
	voe_scene_camera_register(world, 1);
	voe_scene_light_register(world, 1);
	voe_3d_mesh_register(world, 1);
	voe_3d_material_register(world, 1);
	voe_3d_panel_register(world, 1);
	voe_3d_model_register(world, 1);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_camera_add(
		world, entity,
		(voe_scene_camera){ .fov_y = 1.0471976f,
				    .near_plane = 0.1f,
				    .far_plane = 1000.0f }));

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
				   .cast_shadows = true }));

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .position = { 0.0, 0.0, -distance },
				       .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	snprintf(row.path, sizeof(row.path), "%s", path);
	VOE_TEST_CHECK(voe_3d_model_add(world, entity, row));
	return world;
}

// One whole frame of `path` `distance` metres away: the triangles it drew.
static uint64_t triangles(voe_base_arena *arena, voe_render_device *device,
			  const voe_3d_models *models, const char *path,
			  double distance)
{
	voe_ecs_world *world = a_world(arena, path, distance);
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_render_pass_camera camera;
	bool drawing = false;
	uint64_t count;

	frame.models = models;
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return 0;
	VOE_TEST_CHECK(voe_3d_draw_system_shadows(world, device, &frame));
	camera = (voe_render_pass_camera){ .view = frame.view,
					   .light = frame.light,
					   .shadow = frame.shadow };
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	voe_3d_draw_system_run(world, device, arena, frame);
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	count = voe_render_frame_triangle_count(device);
	printf("%s at %.0f m: %llu triangles\n", path, distance,
	       (unsigned long long)count);
	return count;
}

// The sphere loaded as LEVELLED with its levels file and as PLAIN without.
static void load_spheres(voe_base_arena *arena, voe_render_device *device,
			 voe_3d_models *models)
{
	voe_base_error error = VOE_BASE_OK;
	size_t glb_size;
	size_t levels_size = 0;
	const uint8_t *glb = sphere_glb(arena, RINGS, SEGMENTS, false, &glb_size);
	const uint8_t *levels = voe_3d_model_levels_make(
		arena, glb, glb_size, VOE_ASSETS_LEVELS_DEFAULT, &levels_size,
		&error);

	VOE_TEST_CHECK(levels != NULL);
	VOE_TEST_CHECK(voe_3d_models_load_levelled(models, device, LEVELLED, 1,
						   glb, glb_size, levels,
						   levels_size, &error));
	VOE_TEST_CHECK(voe_3d_models_load_levelled(
		models, device, PLAIN, 1, glb, glb_size, NULL, 0, &error));
}

static void draws_use_the_level(voe_base_arena *arena,
				voe_render_device *device,
				const voe_3d_models *models)
{
	uint64_t near = triangles(arena, device, models, LEVELLED, 3.0);
	uint64_t far = triangles(arena, device, models, LEVELLED, 400.0);
	uint64_t whole = triangles(arena, device, models, PLAIN, 400.0);

	VOE_TEST_CHECK(near > 0);
	VOE_TEST_CHECK(far > 0);
	VOE_TEST_CHECK(far * 4 < near);
	VOE_TEST_CHECK(whole > far * 4);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, CAPACITIES, &error);
	voe_3d_models *models;

	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(device != NULL);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}
	models = voe_3d_models_new();
	load_spheres(arena, device, models);

	draws_use_the_level(arena, device, models);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
