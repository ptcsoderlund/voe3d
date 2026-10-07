// The model store's landscapes: a `.landscape`'s text loads as sixteen parts
// on one material and one twin with an empty shape, text that is no landscape
// is kept failed, a brush marks the entry edited and the frame draws its
// chunks transient, the settle makes them static and clears the dirt, a put
// writes its rect, and a rename moves the entry and what lies under it.
//
// THE GRID IS 16 METRES OF 8 CELLS, so a chunk is 2 × 2 cells: 9 vertices and
// 24 indices, a cell 2 m. A brush of radius 3 at the centre touches heights 3
// to 5 on each axis, so chunks 1 and 2 on each axis, 5, 6, 9 and 10, and not
// chunk 0. A geometry's box answers for a live static id, which every part
// has again once settled.
//
// THE DEVICE HOLDS TWO COPIES, for the replace, and one chunk more for a
// settle; and one landscape's chunks transient.
//
// IT SKIPS WHEN THERE IS NO GRAPHICS CARD, because a load uploads.
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

static const voe_render_capacities CAPACITIES = {
	.vertices = 2 * 16 * 9 + 9,
	.indices = 2 * 16 * 24 + 24,
	.geometries = 2 * 16 + 1,
	.objects = 8,
	.shadings = 4,
	.transient_vertices = 16 * 9,
	.transient_indices = 16 * 24,
	.transient_geometries = 16,
	.passes = 1,
};

static bool same(voe_render_geometry a, voe_render_geometry b)
{
	return a.index == b.index && a.generation == b.generation;
}

static bool is_static(voe_render_device *device, voe_render_geometry geometry)
{
	voe_math_float3 min, max;

	return voe_render_geometry_box(device, geometry, &min, &max);
}

static const voe_3d_model_entry *hill(const voe_3d_models *models)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(models, PATH);

	VOE_TEST_CHECK(entry != NULL && entry->loaded && entry->landscape);
	return entry;
}

static void a_landscape_loads_as_sixteen_parts(voe_3d_models *models,
					       voe_render_device *device,
					       voe_base_arena *arena)
{
	voe_assets_landscape flat = voe_assets_landscape_flat(16.0f, 8, arena);
	voe_assets_landscape_text text = voe_assets_landscape_write(&flat, arena);
	voe_base_error error = VOE_BASE_OK;
	const voe_3d_model_entry *entry;
	voe_render_geometry first;

	for (uint64_t stamp = 1; stamp <= 2; stamp++)
		VOE_TEST_CHECK(voe_3d_models_load(
			models, device, PATH, stamp,
			(const uint8_t *)text.text, text.size, &error));
	entry = hill(models);
	if (entry == NULL || entry->landscape == NULL)
		return;
	VOE_TEST_CHECK_INT(entry->stamp, 2);
	VOE_TEST_CHECK_INT(entry->landscape->cells, 8);
	VOE_TEST_CHECK_INT(entry->part_count, 16);
	VOE_TEST_CHECK_INT(entry->shape.vertex_count, 0);
	VOE_TEST_CHECK(!entry->edited && !entry->picture);
	first = entry->parts[0].geometry;
	for (uint32_t p = 0; p < 16; p++) {
		const voe_3d_model_part *part = &entry->parts[p];

		VOE_TEST_CHECK(is_static(device, part->geometry));
		VOE_TEST_CHECK(p == 0 || !same(part->geometry, first));
		VOE_TEST_CHECK_INT(part->material.shading.index,
				   entry->parts[0].material.shading.index);
		VOE_TEST_CHECK_INT(part->faded.index,
				   entry->parts[0].faded.index);
		VOE_TEST_CHECK(part->faded.index != part->material.shading.index);
		VOE_TEST_CHECK_INT(part->material.alpha_mode,
				   VOE_RENDER_ALPHA_OPAQUE);
		VOE_TEST_CHECK_FLOAT(part->material.roughness, 0.9f, 1e-6f);
	}

	VOE_TEST_CHECK(!voe_3d_models_load(models, device, "Assets/bad.landscape",
					   1, (const uint8_t *)"nope", 4,
					   &error));
	entry = voe_3d_models_find(models, "Assets/bad.landscape");
	VOE_TEST_CHECK(entry != NULL && !entry->loaded);
	VOE_TEST_CHECK(entry != NULL && entry->landscape == NULL);
}

static void a_brush_marks_it_edited_and_the_frame_draws_transient(
	voe_3d_models *models, voe_render_device *device,
	voe_base_arena *arena)
{
	const voe_3d_brush raise = { .kind = VOE_3D_BRUSH_RAISE,
				     .radius = 3.0f,
				     .strength = 1.0f,
				     .softness = 0.5f };
	const voe_3d_model_entry *entry = hill(models);
	voe_platform_size size = { SIDE, SIDE };
	voe_render_geometry statics[16];
	voe_3d_landscape_rect rect;
	bool drawing = false;

	if (entry == NULL)
		return;
	rect = voe_3d_models_landscape_brush(models, "Assets/none.landscape",
					     &raise, 0.0f, 0.0f, 0.1f, arena);
	VOE_TEST_CHECK(rect.x0 >= rect.x1);
	rect = voe_3d_models_landscape_brush(models, PATH, &raise, 0.0f, 0.0f,
					     0.1f, arena);
	VOE_TEST_CHECK_INT(rect.x0, 3);
	VOE_TEST_CHECK_INT(rect.x1, 6);
	VOE_TEST_CHECK(entry->edited);
	VOE_TEST_CHECK(entry->landscape->heights[4 * 9 + 4] > 0.0f);

	for (uint32_t p = 0; p < 16; p++)
		statics[p] = entry->parts[p].geometry;
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return;
	voe_3d_models_landscape_frame(models, device, arena);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	for (uint32_t p = 0; p < 16; p++) {
		const bool dirty = p == 5 || p == 6 || p == 9 || p == 10;

		VOE_TEST_CHECK(same(entry->parts[p].geometry, statics[p]) ==
			       !dirty);
		if (!dirty)
			VOE_TEST_CHECK(is_static(device,
						 entry->parts[p].geometry));
	}
}

static void settle_makes_static_and_clears_the_dirt(voe_3d_models *models,
						    voe_render_device *device,
						    voe_base_arena *arena)
{
	const voe_3d_model_entry *entry = hill(models);
	voe_base_error error = VOE_BASE_OK;
	voe_render_geometry settled;

	if (entry == NULL)
		return;
	VOE_TEST_CHECK(voe_3d_models_landscape_settle(models, device, arena,
						      &error));
	for (uint32_t p = 0; p < 16; p++)
		VOE_TEST_CHECK(is_static(device, entry->parts[p].geometry));
	settled = entry->parts[5].geometry;
	VOE_TEST_CHECK(voe_3d_models_landscape_settle(models, device, arena,
						      &error));
	VOE_TEST_CHECK(same(entry->parts[5].geometry, settled));
	VOE_TEST_CHECK(entry->edited);
	voe_3d_models_landscape_saved(models, PATH);
	VOE_TEST_CHECK(!entry->edited);
}

static void put_writes_the_rect(voe_3d_models *models,
				voe_render_device *device,
				voe_base_arena *arena)
{
	const float values[] = { 1.0f, 2.0f, 3.0f, 4.0f };
	const voe_3d_model_entry *entry = hill(models);
	voe_base_error error = VOE_BASE_OK;
	voe_math_float3 min, max;

	if (entry == NULL)
		return;
	voe_3d_models_landscape_put(models, PATH,
				    (voe_3d_landscape_rect){ 0, 0, 2, 2 },
				    values);
	VOE_TEST_CHECK_FLOAT(entry->landscape->heights[0], 1.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(entry->landscape->heights[1], 2.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(entry->landscape->heights[9], 3.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(entry->landscape->heights[10], 4.0f, 0.0f);
	VOE_TEST_CHECK(entry->edited);
	VOE_TEST_CHECK(voe_3d_models_landscape_settle(models, device, arena,
						      &error));
	VOE_TEST_CHECK(voe_render_geometry_box(device, entry->parts[0].geometry,
					       &min, &max));
	VOE_TEST_CHECK_FLOAT(max.y, 4.0f, 1e-6f);
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
	VOE_TEST_CHECK_INT(voe_3d_models_count(models), 2);
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

	a_landscape_loads_as_sixteen_parts(models, device, arena);
	a_brush_marks_it_edited_and_the_frame_draws_transient(models, device,
							      arena);
	settle_makes_static_and_clears_the_dirt(models, device, arena);
	put_writes_the_rect(models, device, arena);
	rename_moves_the_entry(models);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
