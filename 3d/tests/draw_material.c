// A shape wearing a material (0399 point 6), read off a headless frame's
// picture as 3d/tests/draw_system.c reads its red cube: a cube whose `material`
// names a loaded unlit red material reads red at the centre; the same cube with
// an empty path, or a path the store does not hold, reads its own grey.
//
// A MODEL'S PART WEARS ONE TOO: model_data.inc's model, stood as
// 3d/tests/models.c stands it so its green part covers the centre, reads red
// with `materials[1]` naming the red material, and green with it empty or with
// `materials[0]` naming it instead. Part 0 is already red, which is why the
// worn part read is the green part 1: a red part wearing red proves nothing.
//
// UNLIT RED, SO THE CENTRE IS RED WHATEVER THE SUN DOES: a lit grey cube reads
// its three channels alike, and red with nought green and blue is no grey.
//
// THE DEVICE HOLDS THE BUILT-IN SHAPES, the model's two parts and their twins,
// and two records more, the material's own and its blended twin.
//
// IT SKIPS WITHOUT A GRAPHICS CARD, for the reason 3d/tests/import.c gives.
#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/models.h>
#include <3d/panel_component.h>
#include <3d/shape_component.h>
#include <3d/shape_system.h>
#include <assets/material.h>
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
#include <stdlib.h>
#include <string.h>

#include "model_data.inc"

#define SCRATCH (256 * 1024)
#define SIDE 16
#define RED_MATERIAL "Assets/red.material"
#define MODEL "Assets/two.glb"

// A world with a camera at the origin looking down −Z and a sun from above and
// in front.
static voe_ecs_world *a_lit_world(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = 8,
		.component_types = 8,
		.intent_types = 8,
		.structure_requests = 8,
		.structure_bytes = 256,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);
	voe_ecs_entity eye = { 0 };
	voe_ecs_entity sun = { 0 };

	voe_scene_transform_register(world, 8);
	voe_scene_camera_register(world, 2);
	voe_scene_light_register(world, 2);
	voe_3d_mesh_register(world, 8);
	voe_3d_material_register(world, 8);
	voe_3d_panel_register(world, 8);
	voe_3d_shape_register(world, 2);
	// A frame with a store walks the model rows too.
	voe_3d_model_register(world, 1);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &eye));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, eye,
		(voe_scene_transform){ .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_camera_add(
		world, eye,
		(voe_scene_camera){ .fov_y = 1.0471976f,
				    .near_plane = 0.1f,
				    .far_plane = 100.0f }));

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &sun));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, sun,
		(voe_scene_transform){
			.rotation = voe_scene_light_facing(
				(voe_math_float3){ 0.0f, -0.6f, -0.8f }),
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_light_add(
		world, sun,
		(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 3.0f }));
	return world;
}

// The lit world and one cube three metres ahead wearing `material`.
static voe_ecs_world *a_world(voe_base_arena *arena, const char *material)
{
	voe_ecs_world *world = a_lit_world(arena);
	voe_ecs_entity cube = { 0 };
	voe_3d_shape shape = { .kind = VOE_3D_SHAPE_CUBE,
			       .colour = VOE_3D_SHAPE_GREY,
			       .cast_shadows = true };

	strncpy(shape.material, material, sizeof(shape.material) - 1);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &cube));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, cube,
		(voe_scene_transform){ .position = { 0.0, 0.0, -3.0 },
				       .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_3d_shape_add(world, cube, shape));
	return world;
}

// The lit world and one thing wearing MODEL, its `materials[part]` `material`,
// turned and stood as 3d/tests/models.c stands it, five metres out.
static voe_ecs_world *a_model_world(voe_base_arena *arena, uint32_t part,
				    const char *material)
{
	voe_ecs_world *world = a_lit_world(arena);
	voe_ecs_entity thing = { 0 };
	voe_3d_model row = { .path = MODEL, .cast_shadows = true };

	strncpy(row.materials[part], material, sizeof(row.materials[part]) - 1);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &thing));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, thing,
		(voe_scene_transform){
			.position = { 2.0 / 3.0, -4.0 / 3.0, -6.0 },
			.rotation = { 0.0f, -0.70710678f, 0.0f, 0.70710678f },
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_3d_model_add(world, thing, row));
	return world;
}

// One frame of `world` drawn with `models`; the centre pixel's RGBA into
// `centre`. False when there was no picture to read.
static bool centre_of_a_frame(voe_ecs_world *world, voe_render_device *device,
			      voe_base_arena *arena,
			      const voe_3d_models *models, uint8_t centre[4])
{
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_render_pass_camera camera = { .view = frame.view,
					   .light = frame.light };
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;

	frame.models = models;
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return false;
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	voe_3d_draw_system_run(world, device, arena, frame);
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));

	VOE_TEST_CHECK(voe_render_target_read(device, VOE_RENDER_TARGET_WINDOW,
					      arena, &picture, &error));
	if (picture.pixels == NULL)
		return false;
	memcpy(centre,
	       picture.pixels + ((size_t)(picture.height / 2) * picture.width +
				 picture.width / 2) *
					4,
	       4);
	return true;
}

// The cube wearing `material` drawn once; whether its centre reads red.
static void the_cube_reads(voe_render_device *device, voe_base_arena *arena,
			   const voe_3d_shapes *shapes,
			   const voe_3d_models *models, const char *material,
			   bool red)
{
	voe_ecs_world *world = a_world(arena, material);
	uint8_t centre[4] = { 0 };

	voe_3d_shape_system_run(world, shapes);
	if (!centre_of_a_frame(world, device, arena, models, centre))
		return;
	if (red) {
		VOE_TEST_CHECK(centre[0] > 128);
		VOE_TEST_CHECK(centre[1] < 32);
		VOE_TEST_CHECK(centre[2] < 32);
	} else {
		// Grey: lit, so not nought, and its three channels alike.
		VOE_TEST_CHECK(centre[0] > 32);
		VOE_TEST_CHECK(abs((int)centre[0] - (int)centre[1]) < 16);
		VOE_TEST_CHECK(abs((int)centre[0] - (int)centre[2]) < 16);
	}
}

// The model whose `materials[part]` is `material` drawn once; whether its
// centre reads red, else its green part's own green.
static void the_model_reads(voe_render_device *device, voe_base_arena *arena,
			    const voe_3d_models *models, uint32_t part,
			    const char *material, bool red)
{
	voe_ecs_world *world = a_model_world(arena, part, material);
	uint8_t centre[4] = { 0 };

	if (!centre_of_a_frame(world, device, arena, models, centre))
		return;
	if (red) {
		VOE_TEST_CHECK(centre[0] > 128);
		VOE_TEST_CHECK(centre[1] < 32);
	} else {
		VOE_TEST_CHECK(centre[0] < 32);
		VOE_TEST_CHECK(centre[1] > 32);
	}
	VOE_TEST_CHECK(centre[2] < 32);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_capacities capacities = {
		.vertices = VOE_3D_SHAPES_VERTICES + 6,
		.indices = VOE_3D_SHAPES_INDICES + 6,
		.geometries = VOE_3D_SHAPES_GEOMETRIES + 2,
		.objects = 2,
		.shadings = VOE_3D_SHAPES_SHADINGS + 4 + 2,
		.passes = 1,
	};
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, capacities, &error);
	voe_assets_material_file red = voe_assets_material_default();
	voe_3d_material_maps no_maps = { 0 };
	voe_3d_shapes shapes;
	voe_3d_models *models;

	if (device == NULL) {
		VOE_TEST_CHECK(error == VOE_BASE_ERROR_UNAVAILABLE ||
			       error == VOE_BASE_ERROR_UNSUPPORTED);
		printf("skip: %s\n", voe_base_error_string(error));
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}
	VOE_TEST_CHECK(voe_3d_shapes_upload(device, &shapes, &error));

	red.shader = VOE_ASSETS_MATERIAL_UNLIT;
	red.colour[0] = 1.0f;
	red.colour[1] = 0.0f;
	red.colour[2] = 0.0f;
	models = voe_3d_models_new();
	VOE_TEST_CHECK(voe_3d_models_load_material(models, device, RED_MATERIAL,
						   1, &red, no_maps, &error));

	the_cube_reads(device, arena, &shapes, models, RED_MATERIAL, true);
	the_cube_reads(device, arena, &shapes, models, "", false);
	the_cube_reads(device, arena, &shapes, models, "Assets/none.material",
		       false);
	VOE_TEST_CHECK(voe_3d_models_load(models, device, MODEL, 1,
					  TWO_PRIMITIVES_GLB,
					  sizeof(TWO_PRIMITIVES_GLB), &error));
	the_model_reads(device, arena, models, 1, RED_MATERIAL, true);
	the_model_reads(device, arena, models, 1, "", false);
	the_model_reads(device, arena, models, 0, RED_MATERIAL, false);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
