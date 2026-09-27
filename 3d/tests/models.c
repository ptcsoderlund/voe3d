// The model store: the hand-built `.glb` of model_data.inc loaded by path, bytes
// that are no model kept as a failed entry, a file that could not be read kept
// the same way, a path never asked for not there, a path loaded again replacing
// itself, and a clear.
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
// IT SKIPS WHEN THERE IS NO GRAPHICS CARD, because a load uploads.
#include <3d/models.h>
#include <assets/model.h>
#include <base/arena.h>
#include <base/error.h>
#include <math/float3.h>
#include <render/device.h>

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

	voe_3d_models_destroy(models);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
