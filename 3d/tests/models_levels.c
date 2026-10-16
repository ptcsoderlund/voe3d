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
//
// THE DEVICE HOLDS SIXTEEN GEOMETRIES: two copies of the levelled sphere at
// VOE_3D_MODEL_LEVELS levels at most, the two plain ones and the own one's two. A clear or replace
// that leaked a level's geometry runs out within the three rounds of 4.
//
// IT SKIPS WHEN THERE IS NO GRAPHICS CARD, because a load uploads.
#include <3d/model_levels.h>
#include <3d/models.h>

#include <base/arena.h>
#include <base/error.h>
#include <render/device.h>

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

// The files the claims load: the sphere, its levels, its levels made with
// swaps 2, another sphere's levels and the sphere with its own level.
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
	VOE_TEST_CHECK(out.levels != NULL && out.far_levels != NULL &&
		       out.other_levels != NULL);
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

	voe_3d_models_destroy(models);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
