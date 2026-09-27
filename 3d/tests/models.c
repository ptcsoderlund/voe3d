// The model store: the hand-built `.glb` of model_data.inc loaded by path, bytes
// that are no model kept as a failed entry, a file that could not be read kept
// the same way, a path never asked for not there, a path loaded again replacing
// itself, a clear, and a model drawn.
//
// THE DEVICE HOLDS THREE COPIES OF THE MODEL: 6 vertices, 6 indices, 2
// geometries and 2 shading records each. So a hundred loads of one path pass
// only if each replace frees the copy before it; a leak runs out on the third.
//
// THE NODE TRANSFORM IS CHECKED ON THE SHAPE'S VERTICES, which are what a load
// bakes. 3d/tests/import.c says what the file holds: a parent at (1,0,0) turned
// a quarter turn about +Y, and a child at (0,2,0) scaled by two, whose mesh has
// two primitives in two materials. So a point p of the child's mesh lands at
// (1 + 2 p.z, 2 + 2 p.y, -2 p.x), and the model has two parts. The file is read
// again here through `assets` to know each p.
//
// A MODEL IS DRAWN FROM THE FRAME'S STORE (0277 point 3). A camera at the origin
// looks along -Z; a thing wearing the model is turned a quarter turn about -Y,
// so the file's triangles, all in its x = 1 plane facing +X, face the camera,
// and stands so the green part's middle is five metres out on the axis. With
// the store in the frame the centre pixel is not the background; without it,
// and with the thing hidden, it is.
//
// IT SKIPS WHEN THERE IS NO GRAPHICS CARD, because a load uploads.
#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/models.h>
#include <3d/panel_component.h>
#include <assets/model.h>
#include <base/arena.h>
#include <base/error.h>
#include <math/float3.h>
#include <render/device.h>
#include <scene/camera_component.h>
#include <scene/light_component.h>
#include <scene/transform_component.h>
#include <scene/camera_system.h>
#include <scene/light_system.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <stdbool.h>
#include <stdio.h>

#include "model_data.inc"

#define SCRATCH (4 * 1024 * 1024)
#define SIDE 64
#define TOLERANCE 1e-5f

static const voe_render_capacities CAPACITIES = {
	.vertices = 3 * 6,
	.indices = 3 * 6,
	.geometries = 3 * 2,
	.objects = 8,
	.shadings = 3 * 2,
	.passes = 1,
};

static const uint8_t NOT_A_GLB[] = "this is not a model";

static bool load(voe_3d_models *models, voe_render_device *device,
		 const char *path, uint64_t stamp, bool good)
{
	voe_base_error error = VOE_BASE_OK;

	if (good)
		return voe_3d_models_load(models, device, path, stamp,
					  TWO_PRIMITIVES_GLB,
					  sizeof(TWO_PRIMITIVES_GLB), &error);
	return voe_3d_models_load(models, device, path, stamp, NOT_A_GLB,
				  sizeof(NOT_A_GLB), &error);
}

// The same path loaded a hundred times: one entry, loaded, its ids new each
// time; then bad bytes keep it loaded at the new stamp.
static void check_replace(voe_3d_models *models, voe_render_device *device)
{
	const voe_3d_model_entry *entry;
	voe_render_geometry before = { 0 };
	uint32_t count = voe_3d_models_count(models);

	for (uint64_t i = 0; i < 100; i++) {
		VOE_TEST_CHECK(load(models, device, "Assets/again.glb", i,
				    true));
		entry = voe_3d_models_find(models, "Assets/again.glb");
		VOE_TEST_CHECK(entry != NULL);
		if (entry == NULL)
			return;
		VOE_TEST_CHECK(entry->loaded);
		VOE_TEST_CHECK_INT(entry->stamp, i);
		VOE_TEST_CHECK_INT(entry->part_count, 2);
		if (i > 0)
			VOE_TEST_CHECK(entry->parts[0].geometry.index !=
					       before.index ||
				       entry->parts[0].geometry.generation !=
					       before.generation);
		before = entry->parts[0].geometry;
	}
	VOE_TEST_CHECK_INT(voe_3d_models_count(models), count + 1);

	// ---- good then bad: still loaded, the old parts, the new stamp
	VOE_TEST_CHECK(!load(models, device, "Assets/again.glb", 500, false));
	entry = voe_3d_models_find(models, "Assets/again.glb");
	VOE_TEST_CHECK(entry != NULL);
	if (entry == NULL)
		return;
	VOE_TEST_CHECK(entry->loaded);
	VOE_TEST_CHECK_INT(entry->stamp, 500);
	VOE_TEST_CHECK_INT(entry->part_count, 2);
	VOE_TEST_CHECK_INT(entry->parts[0].geometry.index, before.index);
	VOE_TEST_CHECK_INT(entry->parts[0].geometry.generation,
			   before.generation);
	VOE_TEST_CHECK_INT(voe_3d_models_count(models), count + 1);
}

// Every vertex of the shape is the file's own, in walk order, baked.
static void check_baked(voe_base_arena *arena, const voe_3d_model_entry *entry)
{
	voe_base_error error = VOE_BASE_OK;
	voe_assets_model model;
	uint32_t at = 0;

	VOE_TEST_CHECK(voe_assets_model_read_glb(TWO_PRIMITIVES_GLB,
						 sizeof(TWO_PRIMITIVES_GLB),
						 arena, &model, &error));
	VOE_TEST_CHECK_INT(entry->part_count, model.material_count);

	for (uint32_t p = 0; p < model.primitive_count; p++) {
		const voe_assets_primitive *primitive = &model.primitives[p];

		for (uint32_t v = 0; v < primitive->vertex_count; v++) {
			voe_math_float3 source = primitive->positions[v];
			voe_math_float3 baked;

			VOE_TEST_CHECK(at < entry->shape.vertex_count);
			if (at >= entry->shape.vertex_count)
				return;
			baked = entry->shape.vertices[at++].position;
			VOE_TEST_CHECK_FLOAT(baked.x, 1.0f + 2.0f * source.z,
					     TOLERANCE);
			VOE_TEST_CHECK_FLOAT(baked.y, 2.0f + 2.0f * source.y,
					     TOLERANCE);
			VOE_TEST_CHECK_FLOAT(baked.z, -2.0f * source.x,
					     TOLERANCE);
		}
	}
	VOE_TEST_CHECK_INT(entry->shape.vertex_count, at);
	VOE_TEST_CHECK(entry->shape.edge_count > 0);
}

// A camera at the origin and one thing wearing `path`, set in the header.
static voe_ecs_world *a_world(voe_base_arena *arena, const char *path,
			      voe_ecs_entity *thing)
{
	voe_ecs_limits limits = {
		.entities = 4,
		.component_types = 8,
		.intent_types = 8,
		.structure_requests = 8,
		.structure_bytes = 256,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);
	voe_ecs_entity camera = { 0 };
	voe_3d_model model = { 0 };

	voe_scene_transform_register(world, 4);
	voe_scene_camera_register(world, 2);
	voe_scene_light_register(world, 2);
	voe_3d_mesh_register(world, 4);
	voe_3d_material_register(world, 4);
	voe_3d_panel_register(world, 4);
	voe_3d_model_register(world, 4);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &camera));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, camera,
		(voe_scene_transform){ .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_camera_add(
		world, camera,
		(voe_scene_camera){ .fov_y = 1.0471976f,
				    .near_plane = 0.1f,
				    .far_plane = 100.0f }));

	// The green part, (0,2,1) (-2,2,1) (0,0,1) once turned, centred on the
	// axis five metres out.
	VOE_TEST_CHECK(voe_ecs_entity_create(world, thing));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, *thing,
		(voe_scene_transform){
			.position = { 2.0 / 3.0, -4.0 / 3.0, -6.0 },
			.rotation = { 0.0f, -0.70710678f, 0.0f, 0.70710678f },
			.scale = { 1.0f, 1.0f, 1.0f } }));
	snprintf(model.path, sizeof(model.path), "%s", path);
	VOE_TEST_CHECK(voe_3d_model_add(world, *thing, model));
	return world;
}

// The centre pixel of one frame of `world` drawn with `models`, `hidden` left
// out; its RGBA packed.
static uint32_t centre_pixel(voe_ecs_world *world, voe_render_device *device,
			     voe_base_arena *arena, const voe_3d_models *models,
			     voe_ecs_entity hidden)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_render_pass_camera camera;
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;
	const uint8_t *pixel;

	VOE_TEST_CHECK(frame.models == NULL);
	frame.models = models;
	frame.hidden = hidden;
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return 0;
	camera = (voe_render_pass_camera){ frame.view, frame.light, frame.shadow };
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	voe_3d_draw_system_run(world, device, arena, frame);
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	VOE_TEST_CHECK(voe_render_target_read(device, VOE_RENDER_TARGET_WINDOW,
					      arena, &picture, &error));
	if (picture.pixels == NULL)
		return 0;
	pixel = &picture.pixels[((size_t)(SIDE / 2) * SIDE + SIDE / 2) * 4];
	printf("centre %u %u %u %u\n", pixel[0], pixel[1], pixel[2], pixel[3]);
	return (uint32_t)pixel[0] | (uint32_t)pixel[1] << 8 |
	       (uint32_t)pixel[2] << 16 | (uint32_t)pixel[3] << 24;
}

// A thing wearing a loaded model colours the centre pixel with the store in
// the frame; with no store, or hidden, it is the background.
static void check_drawn(voe_3d_models *models, voe_render_device *device)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_ecs_entity thing = { 0 };
	voe_ecs_world *world = a_world(arena, "Assets/two.glb", &thing);
	voe_ecs_entity none = { 0 };
	uint32_t background = centre_pixel(world, device, arena, NULL, none);

	VOE_TEST_CHECK(centre_pixel(world, device, arena, models, none) !=
		       background);
	VOE_TEST_CHECK_INT(centre_pixel(world, device, arena, models, thing),
			   background);
	voe_base_arena_destroy(arena);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	const voe_3d_model_entry *entry;
	voe_render_device *device;
	voe_3d_models *models;

	device = voe_render_device_new_headless(arena, size, CAPACITIES, &error);
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

	// ---- a model loads, and its entry carries the node transform
	VOE_TEST_CHECK(voe_3d_models_load(models, device, "Assets/two.glb", 7,
					  TWO_PRIMITIVES_GLB,
					  sizeof(TWO_PRIMITIVES_GLB), &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_OK);
	entry = voe_3d_models_find(models, "Assets/two.glb");
	VOE_TEST_CHECK(entry != NULL);
	if (entry != NULL) {
		VOE_TEST_CHECK(entry->loaded);
		VOE_TEST_CHECK_INT(entry->stamp, 7);
		check_baked(arena, entry);
	}

	// ---- bytes that are no model: false, MALFORMED, a failed entry
	error = VOE_BASE_OK;
	VOE_TEST_CHECK(!voe_3d_models_load(models, device, "Assets/bad.glb", 3,
					   NOT_A_GLB, sizeof(NOT_A_GLB),
					   &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_MALFORMED);
	entry = voe_3d_models_find(models, "Assets/bad.glb");
	VOE_TEST_CHECK(entry != NULL);
	if (entry != NULL) {
		VOE_TEST_CHECK(!entry->loaded);
		VOE_TEST_CHECK_INT(entry->stamp, 3);
		VOE_TEST_CHECK_INT(entry->part_count, 0);
	}

	// ---- a file that could not be read: a failed entry
	voe_3d_models_fail(models, "Assets/gone.glb", 9);
	entry = voe_3d_models_find(models, "Assets/gone.glb");
	VOE_TEST_CHECK(entry != NULL);
	if (entry != NULL) {
		VOE_TEST_CHECK(!entry->loaded);
		VOE_TEST_CHECK_INT(entry->stamp, 9);
	}

	// ---- a path never asked for is not there
	VOE_TEST_CHECK(voe_3d_models_find(models, "Assets/never.glb") == NULL);
	VOE_TEST_CHECK_INT(voe_3d_models_count(models), 3);
	VOE_TEST_CHECK(voe_3d_models_at(models, 0) ==
		       voe_3d_models_find(models, "Assets/two.glb"));

	// ---- bad then good: the failed entry becomes loaded
	VOE_TEST_CHECK(load(models, device, "Assets/bad.glb", 4, true));
	entry = voe_3d_models_find(models, "Assets/bad.glb");
	VOE_TEST_CHECK(entry != NULL && entry->loaded);
	VOE_TEST_CHECK(entry != NULL && entry->stamp == 4);
	VOE_TEST_CHECK_INT(voe_3d_models_count(models), 3);

	// ---- the store is emptied to make room for the replace test's copies
	voe_3d_models_clear(models, device);
	VOE_TEST_CHECK_INT(voe_3d_models_count(models), 0);
	check_replace(models, device);

	// ---- clear: nothing found, and a load after it works
	voe_3d_models_clear(models, device);
	VOE_TEST_CHECK(voe_3d_models_find(models, "Assets/again.glb") == NULL);
	VOE_TEST_CHECK(voe_3d_models_find(models, "Assets/two.glb") == NULL);
	VOE_TEST_CHECK_INT(voe_3d_models_count(models), 0);
	VOE_TEST_CHECK(load(models, device, "Assets/two.glb", 8, true));
	entry = voe_3d_models_find(models, "Assets/two.glb");
	VOE_TEST_CHECK(entry != NULL && entry->loaded);

	// ---- a thing wearing it is drawn, and not without the store or hidden
	check_drawn(models, device);

	voe_3d_models_destroy(models);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
