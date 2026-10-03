// A fading model is drawn see-through, and a gone one not at all (ADR-0336
// point 3): a camera four metres up looks straight down on a grey ground cube
// whose top is at y = -0.95, under a slanted sun, and a thing wearing the
// hand-built `.glb` of model_data.inc stands on the cube's centre.
//
// THE MODEL FACES UP. Its baked triangles lie in its x = 1 plane facing +X
// (3d/tests/models.c); a quarter turn about +Z faces them +Y, and the thing is
// placed so the green part's middle is on the axis at y = 0, under the
// picture's centre.
//
// THE CENTRE AT THREE FADES. At 0 it is the green part; at 0.5 the part blended
// over the ground, so it differs from both 0 and the same frame of a world with
// no model row; at 1 it equals that frame.
//
// THE SHADOW PASSES: at 1 they issue as many draws as with no model row; at 0.5
// more, since a fading model casts as ever. The sun, the ground and the model
// row say `cast_shadows = true`, so the counts mean something (0324).
//
// IT NEEDS A GRAPHICS CARD AND SKIPS WITH A REASON WITHOUT ONE, as
// 3d/tests/draw_water.c does.
#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/models.h>
#include <3d/panel_component.h>
#include <3d/shadow_cascades.h>
#include <3d/shape_component.h>
#include <3d/shape_system.h>
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

#include "model_data.inc"

#define SCRATCH (4 * 1024 * 1024)
#define SIDE 32
#define PATH "Assets/two.glb"

// The shapes and one copy of the model with its two twins; the ground and the
// model's two parts, each drawn once more into each of the four cascades; four
// shadow passes before the view's one.
static const voe_render_capacities CAPACITIES = {
	.vertices = VOE_3D_SHAPES_VERTICES + 6,
	.indices = VOE_3D_SHAPES_INDICES + 6,
	.geometries = VOE_3D_SHAPES_GEOMETRIES + 2,
	.objects = 3 * (1 + VOE_RENDER_SHADOW_CASCADES),
	.shadings = VOE_3D_SHAPES_SHADINGS + 4,
	.passes = 1 + VOE_RENDER_SHADOW_CASCADES,
	.shadow_size = VOE_3D_SHADOW_TEXELS,
};

// The camera looking down, the sun, the ground and, with `model`, the thing
// wearing PATH, which `thing` is set to.
static voe_ecs_world *a_world(voe_base_arena *arena, const voe_3d_shapes *shapes,
			      bool model, voe_ecs_entity *thing)
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
	voe_3d_model row = { .cast_shadows = true };

	voe_scene_transform_register(world, 8);
	voe_scene_camera_register(world, 1);
	voe_scene_light_register(world, 1);
	voe_3d_mesh_register(world, 4);
	voe_3d_material_register(world, 4);
	voe_3d_panel_register(world, 1);
	voe_3d_shape_register(world, 4);
	voe_3d_model_register(world, 4);

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
				   .fill_intensity = 0.2f,
				   .cast_shadows = true }));

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .position = { 0.0, -1.0, 0.0 },
				       .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 20.0f, 0.1f, 20.0f } }));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, entity,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = { 0.5f, 0.5f, 0.5f },
				.cast_shadows = true }));
	voe_3d_shape_system_run(world, shapes);

	if (!model)
		return world;
	// The green part, (1,2,0) (1,2,2) (1,0,0) baked, is (-2,1,0) (-2,1,2)
	// (0,1,0) once turned; its middle (-4/3, 1, 2/3) moved to the origin.
	VOE_TEST_CHECK(voe_ecs_entity_create(world, thing));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, *thing,
		(voe_scene_transform){
			.position = { 4.0 / 3.0, -1.0, -2.0 / 3.0 },
			.rotation = { 0.0f, 0.0f, 0.70710678f, 0.70710678f },
			.scale = { 1.0f, 1.0f, 1.0f } }));
	snprintf(row.path, sizeof(row.path), "%s", PATH);
	VOE_TEST_CHECK(voe_3d_model_add(world, *thing, row));
	return world;
}

// The thing's row with `fade`, through the model's intent and one run.
static void set_fade(voe_ecs_world *world, voe_ecs_entity thing, float fade)
{
	const voe_3d_model *row = voe_3d_model_get(world, thing);
	voe_3d_model_intent intent = { .entity = thing };

	VOE_TEST_CHECK(row != NULL);
	if (row == NULL)
		return;
	intent.model = *row;
	intent.model.fade = fade;
	VOE_TEST_CHECK(voe_3d_model_submit(world, intent));
	voe_3d_model_system_run(world);
	VOE_TEST_CHECK_FLOAT(voe_3d_model_get(world, thing)->fade, fade, 0.0f);
}

// One frame of `world` from `models`, with its shadows: the shadow passes'
// draws in `cast`, and the picture's centre pixel, rgb packed.
static uint32_t a_frame(voe_ecs_world *world, voe_render_device *device,
			voe_base_arena *arena, const voe_3d_models *models,
			uint32_t *cast)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_render_pass_camera camera;
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;
	const uint8_t *pixel;

	frame.models = models;
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return 0;
	VOE_TEST_CHECK(voe_3d_draw_system_shadows(world, device, &frame));
	*cast = voe_render_frame_draw_count(device);
	camera = (voe_render_pass_camera){ .view = frame.view,
					   .light = frame.light,
					   .shadow = frame.shadow };
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	voe_3d_draw_system_run(world, device, arena, frame);
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	VOE_TEST_CHECK(voe_render_target_read(device, VOE_RENDER_TARGET_WINDOW,
					      arena, &picture, &error));
	if (picture.pixels == NULL)
		return 0;
	pixel = picture.pixels + ((size_t)(picture.height / 2) * picture.width +
				  picture.width / 2) *
					 4;
	printf("centre %u %u %u\n", pixel[0], pixel[1], pixel[2]);
	return (uint32_t)pixel[0] << 16 | (uint32_t)pixel[1] << 8 | pixel[2];
}

static void model_fades(voe_base_arena *arena, voe_render_device *device,
			const voe_3d_shapes *shapes, const voe_3d_models *models)
{
	voe_ecs_entity thing = { 0 };
	voe_ecs_world *bare = a_world(arena, shapes, false, &thing);
	voe_ecs_world *world = a_world(arena, shapes, true, &thing);
	uint32_t bare_cast = 0, solid_cast = 0, half_cast = 0, gone_cast = 0;
	uint32_t none = a_frame(bare, device, arena, models, &bare_cast);
	uint32_t solid = a_frame(world, device, arena, models, &solid_cast);
	uint32_t half;
	uint32_t gone;

	set_fade(world, thing, 0.5f);
	half = a_frame(world, device, arena, models, &half_cast);
	set_fade(world, thing, 1.0f);
	gone = a_frame(world, device, arena, models, &gone_cast);

	// The model is at the centre, half of it at 0.5, none of it at 1.
	VOE_TEST_CHECK(solid != none);
	VOE_TEST_CHECK(half != solid);
	VOE_TEST_CHECK(half != none);
	VOE_TEST_CHECK_INT(gone, none);
	// A fading model casts as ever, a gone one not at all.
	VOE_TEST_CHECK(bare_cast > 0);
	VOE_TEST_CHECK_INT(half_cast, solid_cast);
	VOE_TEST_CHECK(half_cast > bare_cast);
	VOE_TEST_CHECK_INT(gone_cast, bare_cast);
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
	VOE_TEST_CHECK(voe_3d_models_load(models, device, PATH, 1,
					  TWO_PRIMITIVES_GLB,
					  sizeof(TWO_PRIMITIVES_GLB), &error));

	model_fades(arena, device, &shapes, models);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
