// The model store's landscapes: a `.landscape`'s text loads as one part, the
// store's grid on one material and one twin, with a heights texture and an
// empty shape; text that is no landscape is kept failed; a brush marks the
// entry edited and dirty and a frame writes the dirt; a stamp wider than a
// frame's budget takes more than one frame; a put updates the pyramid as a
// fresh build would; a rename moves the entry and what lies under it.
//
// THE GRID IS 16 METRES OF 8 CELLS, a cell 2 m. A brush of radius 3 at the
// centre touches heights 3 to 5 on each axis. The wide one is 1024 cells, so
// a whole put of 1025² heights writes 255 rows a frame and takes 5 frames.
//
// It reads the store's held dirt and pyramid through models_store.h.
//
// IT SKIPS WHEN THERE IS NO GRAPHICS CARD, because a load uploads.
#include "../src/models_store.h"

#include <3d/landscape.h>
#include <3d/models.h>
#include <assets/landscape.h>
#include <base/arena.h>
#include <base/error.h>
#include <render/device.h>

#include <testing/test.h>

#include <stdbool.h>
#include <stdio.h>

#define SCRATCH (1024 * 1024)
#define SIDE 16
#define PATH "Assets/Hill.LANDSCAPE"
#define WIDE "Assets/Wide.landscape"
#define WIDE_CELLS 1024

static const voe_render_capacities CAPACITIES = {
	.vertices = 33 * 33,
	.indices = 32 * 32 * 6,
	.geometries = 1,
	.objects = 8,
	.shadings = 8,
	.passes = 1,
	.heights_texels = VOE_3D_LANDSCAPE_WRITE_TEXELS,
};

static bool same(voe_render_geometry a, voe_render_geometry b)
{
	return a.index == b.index && a.generation == b.generation;
}

static bool is_empty(voe_3d_landscape_rect rect)
{
	return rect.x0 >= rect.x1 || rect.z0 >= rect.z1;
}

static const voe_3d_model_entry *hill(const voe_3d_models *models)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(models, PATH);

	VOE_TEST_CHECK(entry != NULL && entry->loaded && entry->landscape);
	return entry;
}

static entry_held *held_of(voe_3d_models *models, const char *path)
{
	return &models->held[voe_3d_models_index(models, path)];
}

// One frame with nothing but the landscape writes in it.
static bool one_frame(voe_3d_models *models, voe_render_device *device)
{
	voe_platform_size size = { SIDE, SIDE };
	bool drawing = false;

	if (!voe_render_frame_begin(device, size, &drawing) || !drawing)
		return false;
	voe_3d_models_landscape_frame(models, device);
	return voe_render_frame_end(device);
}

static void a_landscape_loads_as_one_part(voe_3d_models *models,
					  voe_render_device *device,
					  voe_base_arena *arena)
{
	voe_assets_landscape flat = voe_assets_landscape_flat(16.0f, 8, arena);
	voe_assets_landscape_text text = voe_assets_landscape_write(&flat, arena);
	voe_base_error error = VOE_BASE_OK;
	const voe_3d_model_entry *entry;
	voe_3d_models_terrain terrain;
	voe_math_float3 min, max;

	for (uint64_t stamp = 1; stamp <= 2; stamp++)
		VOE_TEST_CHECK(voe_3d_models_load(
			models, device, PATH, stamp,
			(const uint8_t *)text.text, text.size, &error));
	entry = hill(models);
	if (entry == NULL || entry->landscape == NULL)
		return;
	VOE_TEST_CHECK_INT(entry->stamp, 2);
	VOE_TEST_CHECK_INT(entry->landscape->cells, 8);
	VOE_TEST_CHECK_INT(entry->part_count, 1);
	VOE_TEST_CHECK_INT(entry->shape.vertex_count, 0);
	VOE_TEST_CHECK(!entry->edited && !entry->picture);
	VOE_TEST_CHECK(entry->parts[0].faded.index !=
		       entry->parts[0].material.shading.index);
	VOE_TEST_CHECK_INT(entry->parts[0].material.alpha_mode,
			   VOE_RENDER_ALPHA_OPAQUE);
	VOE_TEST_CHECK_FLOAT(entry->parts[0].material.roughness, 0.9f, 1e-6f);

	VOE_TEST_CHECK(voe_3d_models_terrain_of(models, entry, &terrain));
	VOE_TEST_CHECK(terrain.heights.index != VOE_RENDER_NO_TEXTURE);
	VOE_TEST_CHECK(terrain.lod != NULL && terrain.lod->levels == 1);
	VOE_TEST_CHECK(same(terrain.grid, entry->parts[0].geometry));
	VOE_TEST_CHECK(voe_render_geometry_box(device, terrain.grid, &min, &max));
	VOE_TEST_CHECK_FLOAT(min.x, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(min.y, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(max.x, 1.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(max.y, 1.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(max.z, 1.0f, 0.0f);

	VOE_TEST_CHECK(!voe_3d_models_load(models, device, "Assets/bad.landscape",
					   1, (const uint8_t *)"nope", 4,
					   &error));
	entry = voe_3d_models_find(models, "Assets/bad.landscape");
	VOE_TEST_CHECK(entry != NULL && !entry->loaded);
	VOE_TEST_CHECK(entry != NULL && entry->landscape == NULL);
	VOE_TEST_CHECK(entry == NULL ||
		       !voe_3d_models_terrain_of(models, entry, &terrain));
}

static void a_brush_then_a_frame_clears_the_dirt(voe_3d_models *models,
						 voe_render_device *device,
						 voe_base_arena *arena)
{
	const voe_3d_brush raise = { .kind = VOE_3D_BRUSH_RAISE,
				     .radius = 3.0f,
				     .strength = 1.0f,
				     .softness = 0.5f };
	const voe_3d_model_entry *entry = hill(models);
	voe_3d_landscape_rect rect;
	entry_held *held;

	if (entry == NULL)
		return;
	held = held_of(models, PATH);
	rect = voe_3d_models_landscape_brush(models, "Assets/none.landscape",
					     &raise, 0.0f, 0.0f, 0.1f, arena);
	VOE_TEST_CHECK(is_empty(rect));
	rect = voe_3d_models_landscape_brush(models, PATH, &raise, 0.0f, 0.0f,
					     0.1f, arena);
	VOE_TEST_CHECK_INT(rect.x0, 3);
	VOE_TEST_CHECK_INT(rect.x1, 6);
	VOE_TEST_CHECK(entry->edited);
	VOE_TEST_CHECK(entry->landscape->heights[4 * 9 + 4] > 0.0f);
	VOE_TEST_CHECK(held->lod.highs[0] > 0.0f);
	VOE_TEST_CHECK_INT(held->dirty.x0, 3);
	VOE_TEST_CHECK_INT(held->dirty.z1, 6);

	VOE_TEST_CHECK(one_frame(models, device));
	VOE_TEST_CHECK(is_empty(held->dirty));
	VOE_TEST_CHECK(entry->edited);
	voe_3d_models_landscape_saved(models, PATH);
	VOE_TEST_CHECK(!entry->edited);
}

// Every node's low and high in `a` as in `b`.
static void same_pyramid(const voe_3d_landscape_lod *a,
			 const voe_3d_landscape_lod *b)
{
	const uint32_t last = a->levels - 1;
	const uint32_t nodes = a->first[last] + a->sides[last] * a->sides[last];

	VOE_TEST_CHECK_INT(a->levels, b->levels);
	for (uint32_t i = 0; a->levels == b->levels && i < nodes; i++) {
		VOE_TEST_CHECK_FLOAT(a->lows[i], b->lows[i], 0.0f);
		VOE_TEST_CHECK_FLOAT(a->highs[i], b->highs[i], 0.0f);
	}
}

static void put_updates_the_pyramid(voe_3d_models *models,
				    voe_render_device *device,
				    voe_base_arena *arena)
{
	const float values[] = { 1.0f, 2.0f, 3.0f, -4.0f };
	const voe_3d_model_entry *entry = hill(models);
	voe_3d_landscape_lod fresh;

	if (entry == NULL)
		return;
	voe_3d_models_landscape_put(models, PATH,
				    (voe_3d_landscape_rect){ 0, 0, 2, 2 },
				    values);
	VOE_TEST_CHECK_FLOAT(entry->landscape->heights[0], 1.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(entry->landscape->heights[1], 2.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(entry->landscape->heights[9], 3.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(entry->landscape->heights[10], -4.0f, 0.0f);
	VOE_TEST_CHECK(entry->edited);
	fresh = voe_3d_landscape_lod_build(entry->landscape, arena);
	same_pyramid(&held_of(models, PATH)->lod, &fresh);
	VOE_TEST_CHECK(one_frame(models, device));
	VOE_TEST_CHECK(is_empty(held_of(models, PATH)->dirty));
}

// 1025 heights a row into 512² texels a frame is 255 rows a frame.
static void a_wide_put_takes_more_than_one_frame(voe_3d_models *models,
						 voe_render_device *device,
						 voe_base_arena *arena)
{
	const uint32_t n = WIDE_CELLS + 1;
	voe_assets_landscape wide =
		voe_assets_landscape_flat(1024.0f, WIDE_CELLS, arena);
	voe_base_error error = VOE_BASE_OK;
	uint32_t frames = 0;
	float *values;
	entry_held *held;

	VOE_TEST_CHECK(voe_3d_models_load_landscape(models, device, WIDE, 1,
						    &wide, &error));
	if (voe_3d_models_index(models, WIDE) == VOE_3D_MODELS)
		return;
	values = voe_base_arena_push(arena, sizeof(float) * n * n);
	for (uint32_t i = 0; i < n * n; i++)
		values[i] = (float)(i % n) * 0.01f;
	voe_3d_models_landscape_put(models, WIDE,
				    (voe_3d_landscape_rect){ 0, 0, n, n },
				    values);
	held = held_of(models, WIDE);
	VOE_TEST_CHECK(one_frame(models, device));
	frames++;
	VOE_TEST_CHECK(!is_empty(held->dirty));
	VOE_TEST_CHECK_INT(held->dirty.z0, VOE_3D_LANDSCAPE_WRITE_TEXELS / n);
	while (!is_empty(held->dirty) && frames < 10) {
		VOE_TEST_CHECK(one_frame(models, device));
		frames++;
	}
	VOE_TEST_CHECK(is_empty(held->dirty));
	VOE_TEST_CHECK_INT(frames, 5);
}

static void rename_moves_the_entry(voe_3d_models *models)
{
	voe_3d_models_rename(models, "Asset", "Nope");
	VOE_TEST_CHECK(voe_3d_models_find(models, PATH) != NULL);
	voe_3d_models_rename(models, "Assets", "Land");
	VOE_TEST_CHECK(voe_3d_models_find(models, PATH) == NULL);
	VOE_TEST_CHECK(voe_3d_models_find(models, "Land/Hill.LANDSCAPE") != NULL);
	VOE_TEST_CHECK(voe_3d_models_find(models, "Land/bad.landscape") != NULL);
	voe_3d_models_rename(models, "Land/Hill.LANDSCAPE",
			     "Land/Mound.landscape");
	VOE_TEST_CHECK(voe_3d_models_find(models, "Land/Mound.landscape") !=
		       NULL);
	VOE_TEST_CHECK_INT(voe_3d_models_count(models), 3);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
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

	a_landscape_loads_as_one_part(models, device, arena);
	a_brush_then_a_frame_clears_the_dirt(models, device, arena);
	put_updates_the_pyramid(models, device, arena);
	a_wide_put_takes_more_than_one_frame(models, device, arena);
	rename_moves_the_entry(models);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
