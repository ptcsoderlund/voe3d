// The model store's levels, on `.glb` files built here (model_glb.inc) and
// levels files made by voe_3d_model_levels_make:
//
// 1. A 64 × 32 sphere with its levels: `level_count` at least 3, not own,
//    level 1's part 0 a live id, level 0's the part's own under an infinite
//    bound, bounds non-increasing, the radius within 1 % of the sphere's 1 and
//    the centre within 1 % of the origin.
// 2. The same sphere with NULL levels, and with the levels made for a different
//    `.glb`: loaded, `level_count` 1.
// 3. The sphere with a second root "Ball_LOD1" and NULL levels: `levels_own`,
//    `level_count` 2, `levels[1].bound` 0.5, level 1's part 0 a live id.
// 4. The levelled sphere loaded again (both copies at once), the store
//    cleared and all four loaded once more, three times over: each load true.
// 5. After a clear, the sphere loaded with a levels file made with swaps 2:
//    `levels[1].bound` half, within 1e-6 relative, that of the sphere loaded
//    with the defaults' file.
// 6. After a clear, 3's sphere loaded with its made file: `level_count` 2,
//    `levels[1].bound` 0.002 × radius ÷ the file's `errors[1]` within 1e-6
//    relative; with `swaps[1]` 2 in its file, half that.
// 7. In 1 and 8, every made level's part geometry has its level-0 part's box,
//    a sharer's box being its base's (0444).
// 8. On a second device whose pools start below the sphere's level 0, the
//    levelled sphere loads with every level its file holds, draws 3 m away
//    and, under a quarter of that, 400 m away; clearing the store after it
//    asserts nothing, made levels freed before the parts they walk.
//
// THE DEVICE HOLDS SIXTEEN GEOMETRIES: two copies of the levelled sphere at
// VOE_3D_MODEL_LEVELS levels at most, the two plain ones and the own one's two. A clear or replace
// that leaked a level's geometry runs out within the three rounds of 4.
//
// IT SKIPS WHEN THERE IS NO GRAPHICS CARD, because a load uploads.
#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/model_levels.h>
#include <3d/models.h>
#include <3d/panel_component.h>

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

#define RINGS 32
#define SEGMENTS 64
#define SIDE 16

static const voe_render_capacities CAPACITIES = {
	.vertices = 24 * 1024,
	.indices = 128 * 1024,
	.geometries = 2 * VOE_3D_MODEL_LEVELS + 2 + 2,
	.objects = 8,
	.shadings = 16,
	.passes = 1,
};

// Room for one levelled sphere's geometries and draws, its pools starting
// below its level 0's 1986 vertices and 11904 indices.
static const voe_render_capacities SMALL_CAPACITIES = {
	.vertices = 1024,
	.indices = 4096,
	.geometries = VOE_3D_MODEL_LEVELS,
	.objects = 4,
	.shadings = 4,
	.passes = 1,
};

// The files the claims load: the sphere, its levels, its levels made with
// swaps 2, another sphere's levels, and the sphere with its own level with
// its levels made at the defaults and with swaps 2.
typedef struct {
	const uint8_t *glb;
	size_t glb_size;
	const uint8_t *levels;
	size_t levels_size;
	const uint8_t *far_levels;
	size_t far_levels_size;
	const uint8_t *other_levels;
	size_t other_levels_size;
	const uint8_t *own;
	size_t own_size;
	const uint8_t *own_levels;
	size_t own_levels_size;
	const uint8_t *own_far_levels;
	size_t own_far_levels_size;
} files;

static files make_files(voe_base_arena *arena)
{
	voe_base_error error = VOE_BASE_OK;
	files out = { 0 };
	size_t other_size;
	const uint8_t *other = sphere_glb(arena, 16, 32, false, &other_size);
	voe_assets_levels far = VOE_ASSETS_LEVELS_DEFAULT;

	for (uint32_t k = 1; k < VOE_ASSETS_LEVELS; k++)
		far.swaps[k] = 2.0f;
	out.glb = sphere_glb(arena, RINGS, SEGMENTS, false, &out.glb_size);
	out.own = sphere_glb(arena, RINGS, SEGMENTS, true, &out.own_size);
	out.levels = voe_3d_model_levels_make(arena, out.glb, out.glb_size,
					      VOE_ASSETS_LEVELS_DEFAULT,
					      &out.levels_size, &error);
	out.far_levels = voe_3d_model_levels_make(arena, out.glb, out.glb_size,
						  far, &out.far_levels_size,
						  &error);
	out.other_levels = voe_3d_model_levels_make(
		arena, other, other_size, VOE_ASSETS_LEVELS_DEFAULT,
		&out.other_levels_size, &error);
	out.own_levels = voe_3d_model_levels_make(arena, out.own, out.own_size,
						  VOE_ASSETS_LEVELS_DEFAULT,
						  &out.own_levels_size, &error);
	out.own_far_levels = voe_3d_model_levels_make(
		arena, out.own, out.own_size, far, &out.own_far_levels_size,
		&error);
	VOE_TEST_CHECK(out.levels != NULL && out.far_levels != NULL &&
		       out.other_levels != NULL && out.own_levels != NULL &&
		       out.own_far_levels != NULL);
	return out;
}

static bool live(voe_render_device *device, voe_render_geometry geometry)
{
	voe_math_float3 min;
	voe_math_float3 max;

	return geometry.generation != 0 &&
	       voe_render_geometry_box(device, geometry, &min, &max);
}

static bool load(voe_3d_models *models, voe_render_device *device,
		 const char *path, const uint8_t *glb, size_t glb_size,
		 const uint8_t *levels, size_t levels_size)
{
	voe_base_error error = VOE_BASE_OK;

	return voe_3d_models_load_levelled(models, device, path, 1, glb,
					   glb_size, levels, levels_size,
					   &error);
}

// 7: each made level's part geometry boxed as its level-0 part, a sharer's box
// being its base's.
static void check_shared_boxes(const voe_3d_model_entry *entry,
			       voe_render_device *device)
{
	for (uint32_t k = 1; k < entry->level_count; k++) {
		for (uint32_t p = 0; p < entry->part_count; p++) {
			const voe_render_geometry made =
				entry->levels[k].geometries[p];
			voe_math_float3 min[2];
			voe_math_float3 max[2];

			if (made.generation == 0)
				continue;
			VOE_TEST_CHECK(voe_render_geometry_box(
				device, made, &min[0], &max[0]));
			VOE_TEST_CHECK(voe_render_geometry_box(
				device, entry->levels[0].geometries[p], &min[1],
				&max[1]));
			VOE_TEST_CHECK(memcmp(&min[0], &min[1], sizeof(min[0])) ==
					       0 &&
				       memcmp(&max[0], &max[1], sizeof(max[0])) ==
					       0);
		}
	}
}

static void check_levelled(const voe_3d_models *models,
			   voe_render_device *device)
{
	const voe_3d_model_entry *entry =
		voe_3d_models_find(models, "Assets/ball.glb");

	VOE_TEST_CHECK(entry != NULL && entry->loaded);
	if (entry == NULL || !entry->loaded)
		return;
	VOE_TEST_CHECK(!entry->levels_own);
	VOE_TEST_CHECK(entry->level_count >= 3);
	VOE_TEST_CHECK(isinf(entry->levels[0].bound));
	VOE_TEST_CHECK_INT(entry->levels[0].geometries[0].index,
			   entry->parts[0].geometry.index);
	VOE_TEST_CHECK(live(device, entry->levels[1].geometries[0]));
	for (uint32_t k = 1; k < entry->level_count; k++)
		VOE_TEST_CHECK(entry->levels[k].bound <=
			       entry->levels[k - 1].bound);
	VOE_TEST_CHECK(fabsf(entry->radius - 1.0f) < 0.01f);
	VOE_TEST_CHECK(voe_math_float3_length(entry->centre) < 0.01f);
	check_shared_boxes(entry, device);
}

// A camera at the origin looking down −Z and one row wearing `path`
// `distance` metres before it.
static voe_ecs_world *a_world(voe_base_arena *arena, const char *path,
			      double distance)
{
	voe_ecs_limits limits = {
		.entities = 2,
		.component_types = 8,
		.intent_types = 8,
		.structure_requests = 8,
		.structure_bytes = 256,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);
	voe_ecs_entity entity = { 0 };
	voe_3d_model row = { 0 };

	voe_scene_transform_register(world, 2);
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
		(voe_scene_transform){ .position = { 0.0, 0.0, -distance },
				       .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	snprintf(row.path, sizeof(row.path), "%s", path);
	VOE_TEST_CHECK(voe_3d_model_add(world, entity, row));
	return world;
}

// One frame of the view's pass with `path` `distance` metres away: the
// triangles it drew.
static uint64_t triangles(voe_base_arena *arena, voe_render_device *device,
			  const voe_3d_models *models, const char *path,
			  double distance)
{
	voe_ecs_world *world = a_world(arena, path, distance);
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_render_pass_camera camera;
	bool drawing = false;

	frame.models = models;
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return 0;
	camera = (voe_render_pass_camera){ .view = frame.view,
					   .light = frame.light,
					   .shadow = frame.shadow };
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	voe_3d_draw_system_run(world, device, arena, frame);
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	return voe_render_frame_triangle_count(device);
}

// 8: the levelled sphere on a device starting with less room than its level 0:
// every level the file holds made, drawn near whole and far under a quarter,
// and the store cleared before the device goes.
static void check_grown(voe_base_arena *arena, const files *f)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device = voe_render_device_new_headless(
		arena, size, SMALL_CAPACITIES, &error);
	voe_3d_models *models = voe_3d_models_new();
	const voe_3d_model_entry *entry;
	voe_3d_model_levels read = {};
	uint64_t near;
	uint64_t far;

	VOE_TEST_CHECK(device != NULL);
	if (device == NULL) {
		voe_3d_models_destroy(models);
		return;
	}
	VOE_TEST_CHECK(voe_3d_model_levels_read(arena, f->levels,
						f->levels_size, f->glb,
						f->glb_size, &read));
	VOE_TEST_CHECK(load(models, device, "Assets/ball.glb", f->glb,
			    f->glb_size, f->levels, f->levels_size));
	entry = voe_3d_models_find(models, "Assets/ball.glb");
	VOE_TEST_CHECK(entry != NULL && entry->loaded);
	if (entry != NULL && entry->loaded) {
		VOE_TEST_CHECK_INT(entry->level_count, read.level_count);
		check_shared_boxes(entry, device);
		near = triangles(arena, device, models, "Assets/ball.glb", 3.0);
		far = triangles(arena, device, models, "Assets/ball.glb",
				400.0);
		VOE_TEST_CHECK(far > 0);
		VOE_TEST_CHECK(far * 4 < near);
	}
	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
	voe_render_device_destroy(device);
}

static void check_level_zero(const voe_3d_models *models, const char *path)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(models, path);

	VOE_TEST_CHECK(entry != NULL && entry->loaded);
	VOE_TEST_CHECK(entry != NULL && entry->level_count == 1);
}

static void check_own(const voe_3d_models *models, voe_render_device *device)
{
	const voe_3d_model_entry *entry =
		voe_3d_models_find(models, "Assets/own.glb");

	VOE_TEST_CHECK(entry != NULL && entry->loaded);
	if (entry == NULL || !entry->loaded)
		return;
	VOE_TEST_CHECK(entry->levels_own);
	VOE_TEST_CHECK_INT(entry->level_count, 2);
	VOE_TEST_CHECK(entry->levels[1].bound == 0.5f);
	VOE_TEST_CHECK(live(device, entry->levels[1].geometries[0]));
}

// The sphere loaded with its defaults' and its swaps-2 levels side by side.
static void check_swap(voe_3d_models *models, voe_render_device *device,
		       const files *f)
{
	const voe_3d_model_entry *normal;
	const voe_3d_model_entry *far;
	float half;

	VOE_TEST_CHECK(load(models, device, "Assets/ball.glb", f->glb,
			    f->glb_size, f->levels, f->levels_size));
	VOE_TEST_CHECK(load(models, device, "Assets/far.glb", f->glb,
			    f->glb_size, f->far_levels, f->far_levels_size));
	normal = voe_3d_models_find(models, "Assets/ball.glb");
	far = voe_3d_models_find(models, "Assets/far.glb");
	VOE_TEST_CHECK(normal != NULL && normal->level_count >= 2);
	VOE_TEST_CHECK(far != NULL && far->level_count >= 2);
	if (normal == NULL || far == NULL || normal->level_count < 2 ||
	    far->level_count < 2)
		return;
	half = normal->levels[1].bound * 0.5f;
	VOE_TEST_CHECK(isfinite(half) && half > 0.0f);
	VOE_TEST_CHECK(fabsf(far->levels[1].bound - half) <= 1e-6f * half);
}

// `path`'s level 1 bound against 0.002 × radius ÷ `levels`' error ÷ `swap`.
static void check_measured(voe_base_arena *arena, const voe_3d_models *models,
			   const char *path, const files *f,
			   const uint8_t *levels, size_t levels_size,
			   float swap)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(models, path);
	voe_3d_model_levels read = {};
	float bound;

	VOE_TEST_CHECK(voe_3d_model_levels_read(arena, levels, levels_size,
						f->own, f->own_size, &read));
	VOE_TEST_CHECK(entry != NULL && entry->loaded);
	if (entry == NULL || !entry->loaded || read.level_count < 2)
		return;
	VOE_TEST_CHECK_INT(entry->level_count, 2);
	bound = 0.002f * entry->radius / read.errors[1] / swap;
	VOE_TEST_CHECK(isfinite(bound) && bound > 0.0f);
	VOE_TEST_CHECK(fabsf(entry->levels[1].bound - bound) <= 1e-6f * bound);
}

// 3's sphere loaded with its two made files side by side.
static void check_own_measured(voe_base_arena *arena, voe_3d_models *models,
			       voe_render_device *device, const files *f)
{
	VOE_TEST_CHECK(load(models, device, "Assets/own.glb", f->own,
			    f->own_size, f->own_levels, f->own_levels_size));
	VOE_TEST_CHECK(load(models, device, "Assets/own_far.glb", f->own,
			    f->own_size, f->own_far_levels,
			    f->own_far_levels_size));
	check_measured(arena, models, "Assets/own.glb", f, f->own_levels,
		       f->own_levels_size, 1.0f);
	check_measured(arena, models, "Assets/own_far.glb", f,
		       f->own_far_levels, f->own_far_levels_size, 2.0f);
}

// All four loaded; false when any load was.
static bool load_all(voe_3d_models *models, voe_render_device *device,
		     const files *f)
{
	bool loaded = load(models, device, "Assets/ball.glb", f->glb,
			   f->glb_size, f->levels, f->levels_size);

	loaded = load(models, device, "Assets/plain.glb", f->glb, f->glb_size,
		      NULL, 0) && loaded;
	loaded = load(models, device, "Assets/other.glb", f->glb, f->glb_size,
		      f->other_levels, f->other_levels_size) && loaded;
	return load(models, device, "Assets/own.glb", f->own, f->own_size,
		    NULL, 0) && loaded;
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(4 * 1024 * 1024);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device;
	voe_3d_models *models;
	files f;

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
	f = make_files(arena);

	// 1 to 3
	VOE_TEST_CHECK(load_all(models, device, &f));
	check_levelled(models, device);
	check_level_zero(models, "Assets/plain.glb");
	check_level_zero(models, "Assets/other.glb");
	check_own(models, device);

	// 4: a replace, a clear and all four again, three times
	for (int round = 0; round < 3; round++) {
		VOE_TEST_CHECK(load(models, device, "Assets/ball.glb", f.glb,
				    f.glb_size, f.levels, f.levels_size));
		voe_3d_models_clear(models, device);
		VOE_TEST_CHECK(load_all(models, device, &f));
	}
	check_levelled(models, device);

	// 5
	voe_3d_models_clear(models, device);
	check_swap(models, device, &f);

	// 6
	voe_3d_models_clear(models, device);
	check_own_measured(arena, models, device, &f);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
	voe_render_device_destroy(device);

	// 8
	check_grown(arena, &f);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
