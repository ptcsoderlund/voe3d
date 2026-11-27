// The model loader's materials against real files: the one-triangle `.glb` of
// glb_build.h and a 1x1 PNG written into a scratch folder in the working
// directory (the build tree), a materials table naming the PNG as maps, and
// game worlds whose shapes and model rows wear them.
//
// THE MATERIALS, on a first world and store: a shape naming a table material
// whose colour map is the PNG loads it, `material`; a shape naming a path not
// in the table is one failure, named, and a second call counts none; with the
// material on disk as text a watch reports nothing and leaves it loaded.
//
// THE BROKEN MAP, on a fresh store with nothing set: a material whose normal
// map is text loads alone as failed, kept so, its map's path in the kept error.
//
// THE LAYERS, on a second store and an empty world: a landscape whose two
// layers name table materials loads both, holds both paths and its painted
// point's bytes as cooked; a layer naming no table material is one failure.
//
// THE PARTS, on a third world and store: a parts table naming a table
// material for the `.glb`'s material 0 makes its part wear it and loads it; a
// table without the `.glb` leaves `wears` empty; a material with an emission
// map loads it into the emissive slot.
//
// The rows, pictures, water and progress cases are tests/models.c; the
// landscapes, foliage and row's landscape cases tests/models_landscapes.c.
//
// The files and the folder are removed at the end, pass or fail. It skips
// when there is no graphics card, because a load uploads.
#include "glb_build.h"

#include <game/models.h>
#include <game/world.h>

#include <3d/models.h>
#include <3d/shape_component.h>

#include <base/arena.h>
#include <base/error.h>
#include <base/report.h>

#include <platform/file.h>
#include <platform/folder.h>

#include <render/device.h>

#include <testing/test.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define FOLDER "game_models_materials_test"
#define GOOD "good.glb"
#define GOOD_ON_DISK FOLDER "/" GOOD
#define PICTURE "puff.png"
#define PICTURE_ON_DISK FOLDER "/" PICTURE
#define MATERIAL "Assets/m.material"
#define NO_MATERIAL "Assets/none.material"
#define GREY_MATERIAL "Assets/grey.material"
#define GLOW_MATERIAL "Assets/glow.material"
#define BROKEN_MATERIAL "Assets/broken.material"
#define BROKEN_PICTURE "broken.png"
#define BROKEN_ON_DISK FOLDER "/" BROKEN_PICTURE
#define LAYERED_LANDSCAPE "layered.landscape"
#define ASSETS_ON_DISK FOLDER "/Assets"
#define MATERIAL_ON_DISK FOLDER "/" MATERIAL
#define CACHE_ON_DISK FOLDER "/Cache"
#define LEVELS_FOLDER_ON_DISK CACHE_ON_DISK "/levels"
#define LEVELS_ON_DISK LEVELS_FOLDER_ON_DISK "/" GOOD ".levels"
#define COPIES_FOLDER_ON_DISK CACHE_ON_DISK "/models"
#define COPY_ON_DISK COPIES_FOLDER_ON_DISK "/" GOOD ".model"
#define MAP_COPIES_FOLDER_ON_DISK CACHE_ON_DISK "/pictures"
#define MAP_COPY_ON_DISK MAP_COPIES_FOLDER_ON_DISK "/" PICTURE ".colour"

// Room for the triangle twice over a reload, the pictures' quad, and the
// shadings of the triangle, the picture and the dot, plus two blended twins:
// one for each triangle part held at once over the reload (ADR-0336 point 2).
// Then a landscape as the store makes it (0396 point 3): the shared grid of
// VOE_3D_LANDSCAPE_NODE_QUADS² quads, its ground and its twin, and the
// heights texture, which takes a texture slot and no capacity. Then a
// material's record and twin, with no geometry, and a second one's. The
// geometries are a model's levels, up to VOE_3D_MODEL_LEVELS, and two more.
#define NODE_SIDE (VOE_3D_LANDSCAPE_NODE_QUADS + 1)
static const voe_render_capacities CAPACITIES = {
	.vertices = 16 + NODE_SIDE * NODE_SIDE,
	.indices = 16 + VOE_3D_LANDSCAPE_NODE_QUADS *
				VOE_3D_LANDSCAPE_NODE_QUADS * 6,
	.geometries = VOE_3D_MODEL_LEVELS + 2,
	.objects = 1,
	.shadings = 10 + 2 + 2 + 2,
	.passes = 1,
};

// A 4-cell, 64 m landscape: a 1.5 m bump in the middle, -0.25 m at a corner.
static const int32_t HILL[25] = {
	-250, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1500,
	0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// A lit, white material with the PNG as its colour map, a grey one with no
// maps, a glowing one with the PNG as its emission map, and one whose normal
// map is text.
static const voe_game_materials MATERIALS = {
	(const voe_game_material[]){ { MATERIAL,
				       { .colour = { 1.0f, 1.0f, 1.0f },
					 .roughness = 0.5f,
					 .repeat = 1.0f,
					 .colormap = PICTURE } },
				     { GREY_MATERIAL,
				       { .colour = { 0.5f, 0.5f, 0.5f },
					 .roughness = 0.5f,
					 .repeat = 1.0f } },
				     { GLOW_MATERIAL,
				       { .colour = { 1.0f, 1.0f, 1.0f },
					 .roughness = 0.5f,
					 .repeat = 1.0f,
					 .emission = { 1.0f, 1.0f, 1.0f },
					 .emission_strength = 1.0f,
					 .emissionmap = PICTURE } },
				     { BROKEN_MATERIAL,
				       { .colour = { 1.0f, 1.0f, 1.0f },
					 .roughness = 0.5f,
					 .repeat = 1.0f,
					 .colormap = PICTURE,
					 .normalmap = BROKEN_PICTURE } } },
	4
};

// No model's parts; the `.glb`'s material 0 wearing MATERIAL.
static const voe_game_parts NO_PARTS = { NULL, 0 };
static const voe_game_parts PARTS = {
	(const voe_game_model_parts[]){
		{ GOOD, { .count = 1, .materials = { MATERIAL } } } },
	1
};

// One plane of a 4-cell landscape's paint: all layer 0 but the middle point,
// 200 of layer 1. Filled by check_layers, since a literal would be 100 bytes.
#define PAINTED 12
static uint8_t paint[25 * 4];

static const voe_game_landscapes LAYERED = {
	(const voe_game_landscape[]){
		{ LAYERED_LANDSCAPE, 64.0f, 4, HILL,
		  (const char *const[]){ MATERIAL, GREY_MATERIAL }, 2, paint, NULL,
		  0, NULL, 0, NULL, NULL } },
	1
};

static const voe_game_landscapes UNKNOWN_LAYER = {
	(const voe_game_landscape[]){
		{ LAYERED_LANDSCAPE, 64.0f, 4, HILL,
		  (const char *const[]){ NO_MATERIAL }, 1, NULL, NULL, 0, NULL,
		  0, NULL, NULL } },
	1
};

// A 1x1 white RGBA PNG: signature, IHDR, one zlib IDAT row, IEND.
static const uint8_t PNG[] = {
	0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
	0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
	0x08, 0x06, 0x00, 0x00, 0x00, 0x1f, 0x15, 0xc4, 0x89, 0x00, 0x00, 0x00,
	0x0b, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0xf8, 0x0f, 0x04, 0x00,
	0x09, 0xfb, 0x03, 0xfd, 0xfb, 0x5e, 0x6b, 0x2b, 0x00, 0x00, 0x00, 0x00,
	0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82,
};

static const uint8_t NOT_A_GLB[] = "this is not a model";

// A grey cube at the origin wearing the material `path`.
static void shade(voe_ecs_world *world, const char *path)
{
	voe_3d_shape shape = { .kind = VOE_3D_SHAPE_CUBE,
			       .colour = VOE_3D_SHAPE_GREY,
			       .cast_shadows = true };

	snprintf(shape.material, sizeof shape.material, "%s", path);
	VOE_TEST_CHECK(voe_3d_shape_add(world, place(world), shape));
}

static bool material_loaded(const voe_3d_models *models)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(models, MATERIAL);

	return entry != NULL && entry->loaded && entry->material;
}

static void check_materials(voe_ecs_world *world, voe_render_device *device,
			    voe_base_arena *scratch)
{
	voe_3d_models *models = voe_3d_models_new();
	voe_game_models_failures failures;

	// ---- a shape naming a table material: loaded, a material
	shade(world, MATERIAL);
	failures = voe_game_models_materials(world, models, device, FOLDER,
					     &MATERIALS, &NO_PARTS, scratch, NULL,
					     NULL);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(material_loaded(models));

	// ---- a path not in the table: one failure, named, then none
	shade(world, NO_MATERIAL);
	failures = voe_game_models_materials(world, models, device, FOLDER,
					     &MATERIALS, &NO_PARTS, scratch, NULL,
					     NULL);
	VOE_TEST_CHECK_INT(failures.count, 1);
	VOE_TEST_CHECK(failures.first != NULL &&
		       strcmp(failures.first, NO_MATERIAL) == 0);
	failures = voe_game_models_materials(world, models, device, FOLDER,
					     &MATERIALS, &NO_PARTS, scratch, NULL,
					     NULL);
	VOE_TEST_CHECK_INT(failures.count, 0);

	// ---- the material on disk as text: a watch leaves it alone
	VOE_TEST_CHECK(voe_platform_folder_create(ASSETS_ON_DISK, NULL));
	VOE_TEST_CHECK(voe_platform_file_write(MATERIAL_ON_DISK, NOT_A_GLB,
					       sizeof(NOT_A_GLB), NULL));
	failures = voe_game_models_watch(models, device, FOLDER, scratch);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(material_loaded(models));

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

// The broken material loaded alone into a fresh store: refused, kept failed,
// the broken map's path in the kept error.
static void broken_load(voe_render_device *device, voe_base_arena *scratch)
{
	voe_3d_models *models = voe_3d_models_new();
	voe_base_error error = VOE_BASE_OK;
	const voe_3d_model_entry *entry;
	const char *kept;

	VOE_TEST_CHECK(voe_platform_file_write(BROKEN_ON_DISK, NOT_A_GLB,
					       sizeof(NOT_A_GLB), NULL));
	voe_base_report_error_clear();
	VOE_TEST_CHECK(!voe_game_models_material_load(models, device, FOLDER,
						      &MATERIALS.materials[3],
						      0, scratch, NULL, &error));
	VOE_TEST_CHECK(error != VOE_BASE_OK);
	entry = voe_3d_models_find(models, BROKEN_MATERIAL);
	VOE_TEST_CHECK(entry != NULL && !entry->loaded);
	kept = voe_base_report_error_first();
	VOE_TEST_CHECK(kept != NULL && strstr(kept, BROKEN_PICTURE) != NULL);
	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

// `path` held in `models` as a loaded material.
static bool holds_material(const voe_3d_models *models, const char *path)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(models, path);

	return entry != NULL && entry->loaded && entry->material;
}

// The layered landscape's grid: both layer paths and the painted point's
// bytes as cooked, the second plane nought.
static void check_layered(const voe_3d_models *models)
{
	const voe_3d_model_entry *entry =
		voe_3d_models_find(models, LAYERED_LANDSCAPE);
	const uint8_t *point;

	VOE_TEST_CHECK(entry != NULL && entry->landscape != NULL);
	if (entry == NULL || entry->landscape == NULL)
		return;
	VOE_TEST_CHECK_INT(entry->landscape->layer_count, 2);
	VOE_TEST_CHECK(strcmp(entry->landscape->layers[0], MATERIAL) == 0);
	VOE_TEST_CHECK(strcmp(entry->landscape->layers[1], GREY_MATERIAL) == 0);
	VOE_TEST_CHECK(entry->landscape->paint != NULL);
	if (entry->landscape->paint == NULL)
		return;
	point = entry->landscape->paint + PAINTED * 4;
	VOE_TEST_CHECK(memcmp(point, paint + PAINTED * 4, 4) == 0);
	VOE_TEST_CHECK(memcmp(point + sizeof(paint), (uint8_t[4]){ 0 }, 4) ==
		       0);
}

static void check_layers(voe_ecs_world *world, voe_render_device *device,
			 voe_base_arena *scratch)
{
	voe_3d_models *models = voe_3d_models_new();
	voe_game_models_failures failures;

	for (uint32_t i = 0; i < 25; i++)
		paint[i * 4] = 255;
	paint[PAINTED * 4] = 55;
	paint[PAINTED * 4 + 1] = 200;

	// ---- two layers naming table materials: both loaded, the paint kept
	failures = voe_game_models_landscapes(models, device, &LAYERED, scratch);
	VOE_TEST_CHECK_INT(failures.count, 0);
	failures = voe_game_models_materials(world, models, device, FOLDER,
					     &MATERIALS, &NO_PARTS, scratch, NULL,
					     NULL);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(holds_material(models, MATERIAL));
	VOE_TEST_CHECK(holds_material(models, GREY_MATERIAL));
	check_layered(models);
	voe_3d_models_clear(models, device);

	// ---- a layer naming no table material: one failure, named
	failures = voe_game_models_landscapes(models, device, &UNKNOWN_LAYER,
					      scratch);
	VOE_TEST_CHECK_INT(failures.count, 0);
	failures = voe_game_models_materials(world, models, device, FOLDER,
					     &MATERIALS, &NO_PARTS, scratch, NULL,
					     NULL);
	VOE_TEST_CHECK_INT(failures.count, 1);
	VOE_TEST_CHECK(failures.first != NULL &&
		       strcmp(failures.first, NO_MATERIAL) == 0);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

// The `.glb`'s part 0 wearing `path`.
static bool wears(const voe_3d_models *models, const char *path)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(models, GOOD);

	return entry != NULL && entry->loaded && entry->part_count == 1 &&
	       strcmp(entry->parts[0].wears, path) == 0;
}

static void check_parts(voe_ecs_world *world, voe_render_device *device,
			voe_base_arena *scratch)
{
	voe_3d_models *models = voe_3d_models_new();
	const voe_3d_model_entry *glow;
	voe_game_models_failures failures;

	// ---- a parts table naming a material for material 0: worn, loaded
	wear(world, GOOD);
	failures = voe_game_models_update(world, models, device, FOLDER, scratch,
					  NULL);
	VOE_TEST_CHECK_INT(failures.count, 0);
	failures = voe_game_models_materials(world, models, device, FOLDER,
					     &MATERIALS, &PARTS, scratch, NULL,
					     NULL);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(wears(models, MATERIAL));
	VOE_TEST_CHECK(material_loaded(models));

	// ---- a table without the `.glb`: nothing worn
	failures = voe_game_models_materials(world, models, device, FOLDER,
					     &MATERIALS, &NO_PARTS, scratch, NULL,
					     NULL);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(wears(models, ""));

	// ---- a material with an emission map: in the emissive slot
	shade(world, GLOW_MATERIAL);
	failures = voe_game_models_materials(world, models, device, FOLDER,
					     &MATERIALS, &NO_PARTS, scratch, NULL,
					     NULL);
	VOE_TEST_CHECK_INT(failures.count, 0);
	glow = voe_3d_models_find(models, GLOW_MATERIAL);
	VOE_TEST_CHECK(glow != NULL && glow->loaded && glow->material);
	if (glow != NULL && glow->loaded)
		VOE_TEST_CHECK(glow->parts[0].material.emissive_texture.index !=
			       VOE_RENDER_NO_TEXTURE);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

// Every file and folder the cases write, deepest first.
static void remove_files(void)
{
	remove(GOOD_ON_DISK);
	remove(PICTURE_ON_DISK);
	remove(BROKEN_ON_DISK);
	remove(MATERIAL_ON_DISK);
	remove(ASSETS_ON_DISK);
	remove(LEVELS_ON_DISK);
	remove(LEVELS_FOLDER_ON_DISK);
	remove(COPY_ON_DISK);
	remove(COPIES_FOLDER_ON_DISK);
	remove(MAP_COPY_ON_DISK);
	remove(MAP_COPIES_FOLDER_ON_DISK);
	remove(CACHE_ON_DISK);
	remove(FOLDER);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1 << 20);
	voe_base_arena *scratch = voe_base_arena_new(1 << 16);
	voe_platform_size size = { 16, 16 };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device;
	uint8_t glb[512];
	uint32_t length = glb_build(glb, sizeof(glb));

	device = voe_render_device_new_headless(arena, size, CAPACITIES, &error);
	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(device != NULL);
		goto released;
	}
	// A folder left by a run that crashed goes first.
	remove_files();
	VOE_TEST_CHECK(voe_platform_folder_create(FOLDER, NULL));
	VOE_TEST_CHECK(voe_platform_file_write(GOOD_ON_DISK, glb, length, NULL));
	VOE_TEST_CHECK(voe_platform_file_write(PICTURE_ON_DISK, PNG,
					       sizeof(PNG), NULL));
	check_materials(voe_game_world_new(arena), device, scratch);
	broken_load(device, scratch);
	check_layers(voe_game_world_new(arena), device, scratch);
	check_parts(voe_game_world_new(arena), device, scratch);
	remove_files();
	voe_render_device_destroy(device);
released:
	voe_base_arena_destroy(scratch);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
