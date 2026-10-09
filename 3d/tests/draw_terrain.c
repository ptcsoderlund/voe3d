// A landscape is drawn by its chosen nodes out to its edge (ADR-0396 points 3,
// 4 and 6): a 4096 m landscape of 2048 cells, flat but for a 60 m ridge along
// its far edge, worn by a thing at the origin, seen by a camera 2 m above its
// centre looking level along -Z at the ridge, far plane 16 km.
//
// THE LENS IS NARROW, 0.2 rad over 64 pixels, so the ridge 2 km off stands
// about nine pixels above the horizon and a flat ground's horizon lies on the
// centre row. The ridge is 128 m deep, so the nodes that far out, 32 m a step,
// still sample it at its full height.
//
// SKY IS WHAT A WORLD WITH NO MODEL ROW DRAWS at the same pixel, under the same
// sun, so ground is any pixel that differs from it.
//
// FOUR CLAIMS. The ridge's pixels above the horizon are ground and the top row
// is sky; a level strip below the horizon is ground with no sky pixel in it;
// the frame draws at least one and at most VOE_3D_LANDSCAPE_NODES objects; a
// row at fade 1 draws nothing and its picture is the bare world's.
//
// IT NEEDS A GRAPHICS CARD AND SKIPS WITH A REASON WITHOUT ONE, as
// 3d/tests/draw_model_fade.c does.
#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/models.h>
#include <3d/panel_component.h>
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
#include <string.h>

#define SCRATCH (96u * 1024u * 1024u)
#define SIDE 64
#define PATH "Assets/Edge.landscape"
#define METRES 4096.0f
#define CELLS 2048u
// Rows 0 to RIDGE_ROWS, the far edge's 128 m, stand RIDGE metres high.
#define RIDGE 60.0f
#define RIDGE_ROWS 64u

// The shared grid, the ground's two records, a node per object and the view's
// one pass.
static const voe_render_capacities CAPACITIES = {
	.vertices = 33 * 33,
	.indices = 32 * 32 * 6,
	.geometries = 1,
	.objects = VOE_3D_LANDSCAPE_NODES,
	.shadings = 2,
	.passes = 1,
};

// The camera looking level along -Z from 2 m up, the sun and, with `model`, the
// thing wearing PATH at the origin, which `thing` is set to.
static voe_ecs_world *a_world(voe_base_arena *arena, bool model,
			      voe_ecs_entity *thing)
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
	voe_3d_model row = { 0 };

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
		(voe_scene_transform){ .position = { 0.0, 2.0, 0.0 },
				       .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_camera_add(
		world, entity,
		(voe_scene_camera){ .fov_y = 0.2f,
				    .near_plane = 0.1f,
				    .far_plane = 16000.0f }));

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){
			.rotation = voe_scene_light_facing(
				(voe_math_float3){ 0.3f, -1.0f, 0.4f }),
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_light_add(
		world, entity,
		(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 3.0f,
				   .fill_colour = { 1.0f, 1.0f, 1.0f },
				   .fill_intensity = 0.2f }));

	if (!model)
		return world;
	VOE_TEST_CHECK(voe_ecs_entity_create(world, thing));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, *thing,
		(voe_scene_transform){ .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	snprintf(row.path, sizeof(row.path), "%s", PATH);
	VOE_TEST_CHECK(voe_3d_model_add(world, *thing, row));
	return world;
}

// The thing's row at fade 1, through the model's intent and one run.
static void gone(voe_ecs_world *world, voe_ecs_entity thing)
{
	const voe_3d_model *row = voe_3d_model_get(world, thing);
	voe_3d_model_intent intent = { .entity = thing };

	VOE_TEST_CHECK(row != NULL);
	if (row == NULL)
		return;
	intent.model = *row;
	intent.model.fade = 1.0f;
	VOE_TEST_CHECK(voe_3d_model_submit(world, intent));
	voe_3d_model_system_run(world);
}

// One frame of `world` from `models`: its picture into `pixels`, SIDE² rgba
// rows top down, and the draws the view issued.
static uint32_t a_frame(voe_ecs_world *world, voe_render_device *device,
			voe_base_arena *arena, const voe_3d_models *models,
			uint8_t *pixels)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_render_pass_camera camera = voe_3d_draw_system_camera(&frame);
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;
	uint32_t draws;

	frame.models = models;
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return 0;
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	voe_3d_draw_system_run(world, device, arena, frame);
	draws = voe_render_frame_draw_count(device);
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	VOE_TEST_CHECK(voe_render_target_read(device, VOE_RENDER_TARGET_WINDOW,
					      arena, &picture, &error));
	VOE_TEST_CHECK(picture.pixels != NULL && picture.width == SIDE &&
		       picture.height == SIDE);
	if (picture.pixels != NULL)
		memcpy(pixels, picture.pixels, (size_t)SIDE * SIDE * 4);
	return draws;
}

// Whether pixel (x, y) of `picture` differs from the bare world's.
static bool ground(const uint8_t *picture, const uint8_t *sky, uint32_t x,
		   uint32_t y)
{
	return memcmp(picture + ((size_t)y * SIDE + x) * 4,
		      sky + ((size_t)y * SIDE + x) * 4, 3) != 0;
}

// Every pixel of row `y` ground or, with `is_ground` false, sky.
static void row_is(const uint8_t *picture, const uint8_t *sky, uint32_t y,
		   bool is_ground)
{
	uint32_t matching = 0;

	for (uint32_t x = 0; x < SIDE; x++)
		matching += ground(picture, sky, x, y) == is_ground;
	printf("row %u: %u of %u %s\n", y, matching, SIDE,
	       is_ground ? "ground" : "sky");
	VOE_TEST_CHECK_INT(matching, SIDE);
}

static void the_ridge_meets_the_sky(voe_base_arena *arena,
				    voe_render_device *device,
				    const voe_3d_models *models)
{
	voe_ecs_entity thing = { 0 };
	voe_ecs_world *bare = a_world(arena, false, &thing);
	voe_ecs_world *world = a_world(arena, true, &thing);
	uint8_t *sky = voe_base_arena_push(arena, (size_t)SIDE * SIDE * 4);
	uint8_t *seen = voe_base_arena_push(arena, (size_t)SIDE * SIDE * 4);
	uint32_t bare_draws = a_frame(bare, device, arena, models, sky);
	uint32_t draws = a_frame(world, device, arena, models, seen);

	printf("draws: %u bare, %u with the landscape\n", bare_draws, draws);
	// The ridge's top 2 km off is ~9.4 pixels above the centre row.
	row_is(seen, sky, 0, false);
	for (uint32_t y = SIDE / 2 - 8; y < SIDE / 2; y++)
		row_is(seen, sky, y, true);
	// Level strips below the horizon, from 400 m out to 20 m.
	for (uint32_t y = SIDE / 2 + 2; y < SIDE; y += 8)
		row_is(seen, sky, y, true);
	VOE_TEST_CHECK_INT(bare_draws, 0);
	VOE_TEST_CHECK(draws > 0);
	VOE_TEST_CHECK(draws <= VOE_3D_LANDSCAPE_NODES);

	gone(world, thing);
	VOE_TEST_CHECK_INT(a_frame(world, device, arena, models, seen), 0);
	VOE_TEST_CHECK(memcmp(seen, sky, (size_t)SIDE * SIDE * 4) == 0);
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
	for (uint32_t r = 0; r <= RIDGE_ROWS; r++)
		for (uint32_t c = 0; c <= CELLS; c++)
			land.heights[r * (CELLS + 1) + c] = RIDGE;
	models = voe_3d_models_new();
	VOE_TEST_CHECK(voe_3d_models_load_landscape(models, device, PATH, 1,
						    &land, &error));

	the_ridge_meets_the_sky(arena, device, models);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
