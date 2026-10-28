// The model store's materials: a `.material` with four 2 × 2 PNG maps, cutout
// 0.4, emission 1 0.5 0 at strength 2 and double-sided, loaded as one part with
// no geometry and a live record and twin, those values in its material, the ORM
// map in the metal-roughness and occlusion slots both and the emission map a
// fourth texture; a set changing cutout and emission, then a frame writing both
// records; the same maps decoded with voe_3d_model_picture_decode and put
// with voe_3d_models_put_material giving a like entry; a colour map alone
// leaving the ORM slots none; a 64 × 64 colour map carrying its levels put as
// one part (put_material_takes_given_levels); a BC7 colour map and a BC5 normal
// map of known blocks put, two more compressed textures
// (put_material_takes_blocks);
// a map that is no picture failing MALFORMED as a failed entry; and a rename
// finding the material at its new path.
//
// THE DEVICE HOLDS FIVE MATERIALS' RECORDS, a record and a twin each, and no
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

#define SCRATCH (1024 * 1024)
#define SIDE 16
#define TOLERANCE 1e-6f

static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 1,
	.shadings = 10,
	.passes = 1,
};

static const uint8_t NOT_A_PICTURE[] = "this is not a picture";

// A material naming four `.png` maps, colour 0.2 0.4 0.6, repeat 4, cutout
// 0.4, emission 1 0.5 0 at strength 2, double-sided.
static voe_assets_material_file four_maps(void)
{
	voe_assets_material_file material = voe_assets_material_default();

	material.colour[0] = 0.2f;
	material.colour[1] = 0.4f;
	material.colour[2] = 0.6f;
	material.repeat = 4.0f;
	material.cutout = 0.4f;
	material.emission[0] = 1.0f;
	material.emission[1] = 0.5f;
	material.emission[2] = 0.0f;
	material.emission_strength = 2.0f;
	material.double_sided = true;
	snprintf(material.emissionmap, sizeof material.emissionmap, "%s",
		 "Assets/brick_glow.png");
	snprintf(material.colormap, sizeof material.colormap, "%s",
		 "Assets/brick.png");
	snprintf(material.normalmap, sizeof material.normalmap, "%s",
		 "Assets/brick_n.png");
	snprintf(material.ormmap, sizeof material.ormmap, "%s",
		 "Assets/brick_orm.png");
	return material;
}

// The loaded entry: a material of one part, no shape, four maps in their
// slots, its cutout, emission and sides, and its record and twin live and
// apart.
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
			   material->metallic_roughness_texture.index);
	VOE_TEST_CHECK(entry->parts[0].faded.index != material->shading.index);

	// ---- cutout, emission × strength, double-sided and a fourth texture
	VOE_TEST_CHECK_INT(material->alpha_mode, VOE_RENDER_ALPHA_CUTOUT);
	VOE_TEST_CHECK_FLOAT(material->alpha_cutoff, 0.4f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(material->emissive.x, 2.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(material->emissive.y, 1.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(material->emissive.z, 0.0f, TOLERANCE);
	VOE_TEST_CHECK(material->double_sided);
	VOE_TEST_CHECK(material->emissive_texture.index != VOE_RENDER_NO_TEXTURE);
	VOE_TEST_CHECK(material->emissive_texture.index !=
		       material->base_colour_texture.index);
	VOE_TEST_CHECK(material->emissive_texture.index !=
		       material->normal_texture.index);
	VOE_TEST_CHECK(material->emissive_texture.index !=
		       material->metallic_roughness_texture.index);
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
	material.cutout = 0.7f;
	material.emission_strength = 1.0f;
	voe_3d_models_material_set(models, "Assets/brick.material", &material);
	entry = voe_3d_models_find(models, "Assets/brick.material");
	VOE_TEST_CHECK(entry != NULL);
	if (entry == NULL)
		return;
	VOE_TEST_CHECK_FLOAT(entry->parts[0].material.roughness, 0.25f,
			     TOLERANCE);
	VOE_TEST_CHECK(entry->parts[0].material.unlit);
	// ---- cutout and emission live, the emission map kept
	VOE_TEST_CHECK_INT(entry->parts[0].material.alpha_mode,
			   VOE_RENDER_ALPHA_CUTOUT);
	VOE_TEST_CHECK_FLOAT(entry->parts[0].material.alpha_cutoff, 0.7f,
			     TOLERANCE);
	VOE_TEST_CHECK_FLOAT(entry->parts[0].material.emissive.x, 1.0f,
			     TOLERANCE);
	VOE_TEST_CHECK_FLOAT(entry->parts[0].material.emissive.y, 0.5f,
			     TOLERANCE);
	VOE_TEST_CHECK(entry->parts[0].material.emissive_texture.index !=
		       VOE_RENDER_NO_TEXTURE);

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

// The four maps decoded by voe_3d_model_picture_decode and put give an entry
// like the load's of the same bytes.
static void check_put(voe_3d_models *models, voe_render_device *device,
		      voe_base_arena *arena,
		      const voe_assets_material_file *material,
		      voe_3d_material_map map)
{
	voe_base_error error = VOE_BASE_OK;
	voe_3d_material_pictures pictures = { 0 };
	const voe_3d_model_entry *entry;

	VOE_TEST_CHECK(voe_3d_model_picture_decode(material->colormap, map.bytes,
						   map.size, arena,
						   &pictures.colour, &error));
	pictures.normal = pictures.orm = pictures.emission = pictures.colour;
	VOE_TEST_CHECK(voe_3d_models_put_material(models, device,
						  "Assets/put.material", 1,
						  material, pictures, &error));
	entry = voe_3d_models_find(models, "Assets/put.material");
	VOE_TEST_CHECK(entry != NULL);
	if (entry != NULL)
		check_loaded(entry);
}

// A 64 × 64 colour map carrying its full chain, put: the put is true and the
// entry a loaded material of one part.
static void put_material_takes_given_levels(voe_3d_models *models,
					    voe_render_device *device,
					    voe_base_arena *arena,
					    const voe_assets_material_file *material)
{
	enum { MAP_SIDE = 64 };
	voe_assets_image image = { .width = MAP_SIDE, .height = MAP_SIDE };
	voe_3d_material_pictures pictures = { 0 };
	voe_base_error error = VOE_BASE_OK;
	const voe_3d_model_entry *entry;

	image.pixels = voe_base_arena_push(arena, MAP_SIDE * MAP_SIDE * 4);
	for (uint32_t i = 0; i < MAP_SIDE * MAP_SIDE * 4; i++)
		image.pixels[i] = (uint8_t)(i * 7);
	pictures.colour = voe_assets_image_mips(arena, &image,
						VOE_ASSETS_IMAGE_COLOUR);
	VOE_TEST_CHECK(pictures.colour.levels > 1);
	VOE_TEST_CHECK(voe_3d_models_put_material(models, device,
						  "Assets/mipped.material", 1,
						  material, pictures, &error));
	entry = voe_3d_models_find(models, "Assets/mipped.material");
	VOE_TEST_CHECK(entry != NULL && entry->loaded && entry->material);
	VOE_TEST_CHECK(entry != NULL && entry->part_count == 1);
}

// A 64 × 64 colour map as BC7 and a normal map as BC5 blocks of known bytes,
// each over its full chain, put: the put is true, the entry a loaded material,
// and the card holds two more compressed material textures. Nothing is drawn
// and read back, so the blocks need not be the picture's.
static void put_material_takes_blocks(voe_3d_models *models,
				      voe_render_device *device,
				      voe_base_arena *arena,
				      const voe_assets_material_file *material)
{
	enum { MAP_SIDE = 64 };
	voe_assets_image image = { .width = MAP_SIDE, .height = MAP_SIDE };
	voe_3d_material_pictures pictures = { 0 };
	voe_base_error error = VOE_BASE_OK;
	const voe_3d_model_entry *entry;
	voe_assets_image colour;
	voe_assets_image normal;
	uint32_t before;

	image.pixels = voe_base_arena_push(arena, MAP_SIDE * MAP_SIDE * 4);
	for (uint32_t i = 0; i < MAP_SIDE * MAP_SIDE * 4; i++)
		image.pixels[i] = (uint8_t)(i * 7);
	colour = voe_assets_image_mips(arena, &image, VOE_ASSETS_IMAGE_COLOUR);
	normal = voe_assets_image_mips(arena, &image, VOE_ASSETS_IMAGE_NORMAL);
	pictures.colour = voe_assets_image_blocks_new(arena, &colour,
						      VOE_ASSETS_IMAGE_COLOUR);
	pictures.normal = voe_assets_image_blocks_new(arena, &normal,
						      VOE_ASSETS_IMAGE_NORMAL);
	for (size_t i = 0; i < voe_assets_image_size(&pictures.colour); i++) {
		pictures.colour.pixels[i] = (uint8_t)(i * 3);
		pictures.normal.pixels[i] = (uint8_t)(i * 5);
	}
	VOE_TEST_CHECK_INT(pictures.colour.format, VOE_ASSETS_IMAGE_BC7);
	VOE_TEST_CHECK_INT(pictures.normal.format, VOE_ASSETS_IMAGE_BC5);

	before = voe_render_texture_memory_smooth(device).compressed;
	VOE_TEST_CHECK(voe_3d_models_put_material(models, device,
						  "Assets/blocks.material", 1,
						  material, pictures, &error));
	entry = voe_3d_models_find(models, "Assets/blocks.material");
	VOE_TEST_CHECK(entry != NULL && entry->loaded && entry->material);
	VOE_TEST_CHECK_INT(voe_render_texture_memory_smooth(device).compressed,
			   before + 2);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	uint8_t pixels[2 * 2 * 4] = { 255, 0, 0, 255, 0, 255, 0, 255,
				      0, 0, 255, 255, 255, 255, 255, 255 };
	voe_assets_image image = { .width = 2, .height = 2, .pixels = pixels };
	voe_platform_size size = { SIDE, SIDE };
	voe_assets_material_file material = four_maps();
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

	// ---- four maps load as one live part
	VOE_TEST_CHECK(voe_3d_models_load_material(
		models, device, "Assets/brick.material", 1, &material,
		(voe_3d_material_maps){ map, map, map, map }, &error));
	entry = voe_3d_models_find(models, "Assets/brick.material");
	VOE_TEST_CHECK(entry != NULL);
	if (entry != NULL)
		check_loaded(entry);

	check_set(models, device, material);

	// ---- the same maps decoded first and put: an entry like the load's
	check_put(models, device, arena, &material, map);

	// ---- a colour map alone leaves the ORM map's two slots none
	VOE_TEST_CHECK(voe_3d_models_load_material(
		models, device, "Assets/plain.material", 1, &material,
		(voe_3d_material_maps){ .colour = map }, &error));
	entry = voe_3d_models_find(models, "Assets/plain.material");
	VOE_TEST_CHECK(entry != NULL);
	if (entry != NULL) {
		VOE_TEST_CHECK_INT(
			entry->parts[0].material.metallic_roughness_texture.index,
			VOE_RENDER_NO_TEXTURE);
		VOE_TEST_CHECK_INT(entry->parts[0].material.occlusion_texture.index,
				   VOE_RENDER_NO_TEXTURE);
	}

	// ---- a colour map carrying its levels goes up with them
	put_material_takes_given_levels(models, device, arena, &material);

	// ---- a BC7 colour map and a BC5 normal map go up as their blocks
	put_material_takes_blocks(models, device, arena, &material);

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
