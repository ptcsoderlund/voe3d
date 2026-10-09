// The model store's materials: a `.material` with three 2 × 2 PNG maps loaded
// as one part with no geometry and a live record and twin; a set then a frame
// writing both records; a map that is no picture failing MALFORMED as a failed
// entry; and a rename finding the material at its new path.
//
// THE DEVICE HOLDS TWO MATERIALS' RECORDS, a record and a twin each, and no
// geometry: a material's part draws nothing of its own.
//
// IT SKIPS WHEN THERE IS NO GRAPHICS CARD, because a load uploads.
#include <3d/models.h>
#include <assets/image.h>
#include <assets/material.h>
#include <base/arena.h>
#include <base/error.h>
#include <render/device.h>

#include <testing/test.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define SCRATCH (1024 * 1024)
#define SIDE 16
#define TOLERANCE 1e-6f

static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 1,
	.shadings = 4,
	.passes = 1,
};

static const uint8_t NOT_A_PICTURE[] = "this is not a picture";

// A material naming three `.png` maps, colour 0.2 0.4 0.6, repeat 4.
static voe_assets_material_file three_maps(void)
{
	voe_assets_material_file material = voe_assets_material_default();

	material.colour[0] = 0.2f;
	material.colour[1] = 0.4f;
	material.colour[2] = 0.6f;
	material.repeat = 4.0f;
	strcpy(material.colour_map, "Assets/brick.png");
	strcpy(material.normal_map, "Assets/brick_n.png");
	strcpy(material.roughness_map, "Assets/brick_r.png");
	return material;
}

// The loaded entry: a material of one part, no shape, three maps in their
// slots and its record and twin live and apart.
static void check_loaded(const voe_3d_model_entry *entry)
{
	const voe_3d_material *material = &entry->parts[0].material;

	VOE_TEST_CHECK(entry->loaded && entry->material);
	VOE_TEST_CHECK_INT(entry->part_count, 1);
	VOE_TEST_CHECK_INT(entry->shape.vertex_count, 0);
	VOE_TEST_CHECK_FLOAT(material->base_colour.y, 0.4f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(material->uv_repeat, 4.0f, TOLERANCE);
	VOE_TEST_CHECK(material->base_colour_texture.index !=
		       VOE_RENDER_NO_TEXTURE);
	VOE_TEST_CHECK(material->normal_texture.index != VOE_RENDER_NO_TEXTURE);
	VOE_TEST_CHECK(material->metallic_roughness_texture.index !=
		       VOE_RENDER_NO_TEXTURE);
	VOE_TEST_CHECK_INT(material->occlusion_texture.index,
			   VOE_RENDER_NO_TEXTURE);
	VOE_TEST_CHECK(entry->parts[0].faded.index != material->shading.index);
}

// A set takes the values, and a frame writing the marked records passes.
static void check_set(voe_3d_models *models, voe_render_device *device,
		      voe_assets_material_file material)
{
	voe_platform_size size = { SIDE, SIDE };
	const voe_3d_model_entry *entry;
	bool drawing = false;

	material.roughness = 0.25f;
	material.shader = VOE_ASSETS_MATERIAL_UNLIT;
	voe_3d_models_material_set(models, "Assets/brick.material", &material);
	entry = voe_3d_models_find(models, "Assets/brick.material");
	VOE_TEST_CHECK(entry != NULL);
	if (entry == NULL)
		return;
	VOE_TEST_CHECK_FLOAT(entry->parts[0].material.roughness, 0.25f,
			     TOLERANCE);
	VOE_TEST_CHECK(entry->parts[0].material.unlit);

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	if (!drawing)
		return;
	voe_3d_models_material_frame(models, device);
	VOE_TEST_CHECK(voe_render_frame_end(device));

	// ---- a path that is no material is let be
	voe_3d_models_material_set(models, "Assets/none.material", &material);
	VOE_TEST_CHECK(voe_3d_models_find(models, "Assets/none.material") ==
		       NULL);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	uint8_t pixels[2 * 2 * 4] = { 255, 0, 0, 255, 0, 255, 0, 255,
				      0, 0, 255, 255, 255, 255, 255, 255 };
	voe_assets_image image = { .width = 2, .height = 2, .pixels = pixels };
	voe_platform_size size = { SIDE, SIDE };
	voe_assets_material_file material = three_maps();
	voe_base_error error = VOE_BASE_OK;
	const voe_3d_model_entry *entry;
	voe_assets_bytes png = { 0 };
	voe_render_device *device;
	voe_3d_material_map map;
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
	VOE_TEST_CHECK(voe_assets_png_encode(arena, image, &png, &error));
	map = (voe_3d_material_map){ .bytes = png.bytes, .size = png.count };

	// ---- three maps load as one live part
	VOE_TEST_CHECK(voe_3d_models_load_material(
		models, device, "Assets/brick.material", 1, &material,
		(voe_3d_material_maps){ map, map, map }, &error));
	entry = voe_3d_models_find(models, "Assets/brick.material");
	VOE_TEST_CHECK(entry != NULL);
	if (entry != NULL)
		check_loaded(entry);

	check_set(models, device, material);

	// ---- a map that is no picture: MALFORMED, a failed entry
	error = VOE_BASE_OK;
	map = (voe_3d_material_map){ .bytes = NOT_A_PICTURE,
				     .size = sizeof(NOT_A_PICTURE) };
	VOE_TEST_CHECK(!voe_3d_models_load_material(
		models, device, "Assets/junk.material", 2, &material,
		(voe_3d_material_maps){ .colour = map }, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_MALFORMED);
	entry = voe_3d_models_find(models, "Assets/junk.material");
	VOE_TEST_CHECK(entry != NULL && !entry->loaded && !entry->material);
	VOE_TEST_CHECK(entry != NULL && entry->part_count == 0);

	// ---- a rename finds it at the new path
	voe_3d_models_rename(models, "Assets/brick.material",
			     "Assets/wall.material");
	VOE_TEST_CHECK(voe_3d_models_find(models, "Assets/brick.material") ==
		       NULL);
	entry = voe_3d_models_find(models, "Assets/wall.material");
	VOE_TEST_CHECK(entry != NULL && entry->loaded && entry->material);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
