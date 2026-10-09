// A landscape casts by its nodes (ADR-0396 point 4): every shadow pass of a
// frame draws the ground the view's eye chooses, so a ridge shadows the ground
// behind it.
//
// A 512 m landscape of 256 cells, 2 m a cell, flat at nought but for a 40 m
// ridge along X across its middle, rows 124 to 132, z −8 to 8 m. A low sun
// shines along +Z, 26.6° up, casting with no fill and no bounce, so the ridge's
// shadow runs from z 8 to about 88 m and a shadowed surface is black.
//
// ONE PICTURE, TWO GROUND PIXELS. A camera 150 m above the origin looks
// straight down, a vertical field of 1 rad over 64 pixels, world -Z at the top
// of the picture. Ground at z = ±40 m lies 0.488 of the half field from the
// middle, 15.6 pixels: z = +40, behind the ridge from the sun, on row 48, and
// z = −40, in front of it, on row 16, both on the middle column. Each sight
// line passes the ridge's top 29 m to its side. The first reads darker than
// the second by more than 32 levels, in the same frame, and the four cascades
// each draw at least one node.
//
// IT NEEDS A GRAPHICS CARD AND SKIPS WITH A REASON WITHOUT ONE, as
// 3d/tests/draw_terrain.c does.
#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/models.h>
#include <3d/panel_component.h>
#include <3d/shadow_cascades.h>
#include <assets/landscape.h>
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

#define SCRATCH (16u * 1024u * 1024u)
#define SIDE 64
#define PATH "Assets/Ridge.landscape"
#define METRES 512.0f
#define CELLS 256u
#define RIDGE 40.0f
#define RIDGE_FIRST 124u
#define RIDGE_LAST 132u
// The two ground pixels, worked out in the header.
#define BEHIND_Y 48
#define FRONT_Y 16
#define MIDDLE_X 32
// At most 8 × 8 leaves of 32 cells, each a node in the view and each cascade.
#define MOST_NODES 64u

// The shared grid, the ground's two records, every node drawn into the four
// cascades and the view, and those five passes.
static const voe_render_capacities CAPACITIES = {
	.vertices = 33 * 33,
	.indices = 32 * 32 * 6,
	.geometries = 1,
	.objects = (1 + VOE_RENDER_SHADOW_CASCADES) * MOST_NODES,
	.shadings = 2,
	.passes = 1 + VOE_RENDER_SHADOW_CASCADES,
	.shadow_size = VOE_3D_SHADOW_TEXELS,
};

// The camera looking down from 150 m, the low casting sun and the thing
// wearing PATH at the origin.
static voe_ecs_world *a_world(voe_base_arena *arena)
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
		(voe_scene_transform){
			.position = { 0.0, 150.0, 0.0 },
			.rotation = { -0.70710678f, 0.0f, 0.0f, 0.70710678f },
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_camera_add(
		world, entity,
		(voe_scene_camera){ .fov_y = 1.0f,
				    .near_plane = 0.1f,
				    .far_plane = 1000.0f }));

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){
			.rotation = voe_scene_light_facing(
				(voe_math_float3){ 0.0f, -0.5f, 1.0f }),
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_light_add(
		world, entity,
		(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 3.0f,
				   .bounces = 0,
				   .cast_shadows = true }));

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	snprintf(row.path, sizeof(row.path), "%s", PATH);
	VOE_TEST_CHECK(voe_3d_model_add(world, entity, row));
	return world;
}

// The ridge's shadow darkens the ground behind it, not in front, in one frame.
static void the_ridge_shadows_the_ground(voe_base_arena *arena,
					 voe_render_device *device,
					 const voe_3d_models *models)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_ecs_world *world = a_world(arena);
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_render_pass_camera camera;
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;
	uint32_t cast;
	uint8_t behind;
	uint8_t front;

	frame.models = models;
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return;
	VOE_TEST_CHECK(voe_3d_draw_system_shadows(world, device, &frame));
	cast = voe_render_frame_draw_count(device);
	VOE_TEST_CHECK_INT(frame.shadow.count, VOE_RENDER_SHADOW_CASCADES);
	camera = (voe_render_pass_camera){ .view = frame.view,
					   .light = frame.light,
					   .shadow = frame.shadow,
					   .points = frame.points };
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	voe_3d_draw_system_run(world, device, arena, frame);
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	VOE_TEST_CHECK(voe_render_target_read(device, VOE_RENDER_TARGET_WINDOW,
					      arena, &picture, &error));
	VOE_TEST_CHECK(picture.pixels != NULL && picture.width == SIDE &&
		       picture.height == SIDE);
	if (picture.pixels == NULL)
		return;
	behind = picture.pixels[((size_t)BEHIND_Y * SIDE + MIDDLE_X) * 4];
	front = picture.pixels[((size_t)FRONT_Y * SIDE + MIDDLE_X) * 4];
	printf("cascade draws %u; ground behind the ridge %u, in front %u\n",
	       cast, behind, front);
	VOE_TEST_CHECK(cast >= VOE_RENDER_SHADOW_CASCADES);
	VOE_TEST_CHECK(cast <= VOE_RENDER_SHADOW_CASCADES * MOST_NODES);
	VOE_TEST_CHECK(behind + 32 < front);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, CAPACITIES, &error);
	voe_assets_landscape land;
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
	land = voe_assets_landscape_flat(METRES, CELLS, arena);
	for (uint32_t r = RIDGE_FIRST; r <= RIDGE_LAST; r++)
		for (uint32_t c = 0; c <= CELLS; c++)
			land.heights[r * (CELLS + 1) + c] = RIDGE;
	models = voe_3d_models_new();
	VOE_TEST_CHECK(voe_3d_models_load_landscape(models, device, PATH, 1,
						    &land, &error));

	the_ridge_shadows_the_ground(arena, device, models);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
