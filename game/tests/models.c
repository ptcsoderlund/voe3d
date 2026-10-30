// The model loader against real files: a one-triangle `.glb` built here, JSON
// and BIN chunks by hand, written into a scratch folder in the working
// directory (the build tree), and a game world whose model rows name it.
//
// THE CASES, in order on one store: a row naming the file updates with no
// failure and a loaded entry; a second row naming a missing file gives one
// failure, named; unchanged, the watch gathers no entry; the file rewritten
// as text, it gathers that one alone, reports one failure and keeps it
// loaded; rewritten valid, a watch reports none and the entry is loaded at
// the new stamp. That world has no emitter, so no dot is loaded.
//
// THE PICTURES, on a second world and store: an emitter naming a 1x1 PNG,
// inline bytes, loads a picture entry at
// its path and the dot at ""; a second emitter naming a missing file is one
// failure, named.
//
// THE WATER, on a third world and store: with no water an update loads no
// water record; with one, it loads it.
//
// THE PROGRESS, on a fourth world and store: two rows naming the `.glb` update
// with a progress to total 1 and done 1; with stop already asked an update
// reads nothing, and a second one without a progress loads the file.
//
// THE LANDSCAPES, on a fifth store: a 4-cell table loads at stamp 0 with its
// millimetres as metres, one part; with a text file at its path a watch reports nothing
// and leaves it loaded at 0.
//
// THE MATERIALS, on a sixth world and store: a shape naming a table material
// whose colour map is the PNG loads it, `material`; a shape naming a path not
// in the table is one failure, named, and a second call counts none; with the
// material on disk as text a watch reports nothing and leaves it loaded.
//
// THE BROKEN MAP, on a fresh store with nothing set: a material whose normal
// map is text loads alone as failed, kept so, its map's path in the kept error.
//
// THE LAYERS, on a seventh store and an empty world: a landscape whose two
// layers name table materials loads both, holds both paths and its painted
// point's bytes as cooked; a layer naming no table material is one failure.
//
// THE PARTS, on an eighth world and store: a parts table naming a table
// material for the `.glb`'s material 0 makes its part wear it and loads it; a
// table without the `.glb` leaves `wears` empty; a material with an emission
// map loads it into the emissive slot.
//
// THE FOLIAGE, on a ninth store and an empty world: a landscape naming the
// `.glb` as its one kind, at a base size of 2×, with two copies, makes an
// update load the kind and the grid hold both copies as cooked, placed at
// twice their own scale; with no looks, at their own; two layers, the kind's
// look with spreads and casting none and layer 1 growing it, load with that look
// and those grows on the grid; a kind naming a missing file is one failure, named.
//
// THE ROW'S LANDSCAPE, on a tenth world and store: a model row naming a
// `.landscape` on disk whose one kind is the `.glb` makes one update with a
// progress load the kind too, total and done 2.
//
// The levels cases are tests/models_levels.c; the `.glb` and the world helpers
// both use are tests/glb_build.h.
//
// The files and the folder are removed at the end, pass or fail. It skips
// when there is no graphics card, because a load uploads.
#include "glb_build.h"

#include "../src/models_batch.h"

#include <game/models.h>
#include <game/progress.h>
#include <game/world.h>

#include <3d/emitter_component.h>
#include <3d/model_component.h>
#include <3d/models.h>
#include <3d/models_foliage.h>
#include <3d/shape_component.h>
#include <3d/water_component.h>

#include <assets/landscape.h>

#include <base/arena.h>
#include <base/error.h>
#include <base/report.h>

#include <math/quat.h>

#include <platform/file.h>
#include <platform/folder.h>

#include <render/device.h>

#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define FOLDER "game_models_test"
#define GOOD "good.glb"
#define MISSING "missing.glb"
#define GOOD_ON_DISK FOLDER "/" GOOD
#define PICTURE "puff.png"
#define NO_PICTURE "missing.png"
#define PICTURE_ON_DISK FOLDER "/" PICTURE
#define LANDSCAPE "hill.landscape"
#define LANDSCAPE_ON_DISK FOLDER "/" LANDSCAPE
#define ROW_LANDSCAPE "forest.landscape"
#define ROW_LANDSCAPE_ON_DISK FOLDER "/" ROW_LANDSCAPE
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

static const voe_game_landscapes HILLS = {
	(const voe_game_landscape[]){
		{ LANDSCAPE, 64.0f, 4, HILL, NULL, 0, NULL, NULL, 0, NULL, 0,
		  NULL, NULL } },
	1
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
// 200 of layer 1. Filled by main, since a literal would be 100 bytes.
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

// Two copies of kind 0: one turned a quarter, one leaning along the ground.
static const voe_assets_landscape_copy COPIES[2] = {
	{ .x = 1000, .z = -2000, .turn = 16384, .scale = 1000 },
	{ .x = -5000, .z = 3000, .scale = 1500, .lean = 255 },
};

// The copies with their kind at twice its size, and with no looks: 1×.
static const voe_game_landscapes FOREST = {
	(const voe_game_landscape[]){ { LANDSCAPE, 64.0f, 4, HILL, NULL, 0, NULL,
					(const char *const[]){ GOOD }, 1, COPIES,
					2,
					(const voe_assets_landscape_look[]){
						{ .size = 2000 } },
					NULL } },
	1
};

// Two layers and the `.glb` as its one kind, with spreads and casting none,
// grown by layer 1 alone.
static const voe_assets_landscape_look MEADOW_LOOK = {
	.size = 1500, .size_vary = 40, .turn_vary = 90, .tilt = 60,
	.shadowless = true
};
static const voe_game_landscapes MEADOW = {
	(const voe_game_landscape[]){
		{ LANDSCAPE, 64.0f, 4, HILL,
		  (const char *const[]){ MATERIAL, GREY_MATERIAL }, 2, paint,
		  (const char *const[]){ GOOD }, 1, NULL, 0, &MEADOW_LOOK,
		  (const uint16_t[]){ 0, 40 } } },
	1
};

static const voe_game_landscapes GROVE = {
	(const voe_game_landscape[]){ { LANDSCAPE, 64.0f, 4, HILL, NULL, 0, NULL,
					(const char *const[]){ GOOD }, 1, COPIES,
					2, NULL, NULL } },
	1
};

static const voe_game_landscapes THICKET = {
	(const voe_game_landscape[]){ { LANDSCAPE, 64.0f, 4, HILL, NULL, 0, NULL,
					(const char *const[]){ MISSING }, 1,
					NULL, 0, NULL, NULL } },
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

// A thing at the origin emitting with the texture `path`.
static void emit(voe_ecs_world *world, const char *path)
{
	voe_3d_emitter emitter = { .playing = true };

	snprintf(emitter.texture, sizeof emitter.texture, "%s", path);
	VOE_TEST_CHECK(voe_3d_emitter_add(world, place(world), emitter));
}

static void check_pictures(voe_ecs_world *world, voe_render_device *device,
			   voe_base_arena *scratch)
{
	voe_3d_models *models = voe_3d_models_new();
	const voe_3d_model_entry *entry;
	voe_game_models_failures failures;

	// ---- an emitter naming a PNG: a picture entry and the dot
	emit(world, PICTURE);
	failures = voe_game_models_update(world, models, device, FOLDER, scratch,
					  NULL);
	VOE_TEST_CHECK_INT(failures.count, 0);
	entry = voe_3d_models_find(models, PICTURE);
	VOE_TEST_CHECK(entry != NULL && entry->loaded && entry->picture);
	entry = voe_3d_models_find(models, "");
	VOE_TEST_CHECK(entry != NULL && entry->loaded && entry->picture);
	VOE_TEST_CHECK_INT(voe_3d_models_count(models), 1);

	// ---- an emitter naming a missing file: one failure, named
	emit(world, NO_PICTURE);
	failures = voe_game_models_update(world, models, device, FOLDER, scratch,
					  NULL);
	VOE_TEST_CHECK_INT(failures.count, 1);
	VOE_TEST_CHECK(failures.first != NULL &&
		       strcmp(failures.first, NO_PICTURE) == 0);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

static void check_water(voe_ecs_world *world, voe_render_device *device,
			voe_base_arena *scratch)
{
	voe_3d_models *models = voe_3d_models_new();
	voe_game_models_failures failures;

	// ---- no water: no record
	failures = voe_game_models_update(world, models, device, FOLDER, scratch,
					  NULL);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(voe_3d_models_water(models) == NULL);

	// ---- one water: the record
	VOE_TEST_CHECK(voe_3d_water_add(world, place(world),
					(voe_3d_water){ .width = 20.0f,
							.length = 20.0f }));
	failures = voe_game_models_update(world, models, device, FOLDER, scratch,
					  NULL);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(voe_3d_models_water(models) != NULL);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

static bool loaded_at(const voe_3d_models *models, uint64_t *stamp)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(models, GOOD);

	if (entry == NULL)
		return false;
	*stamp = entry->stamp;
	return entry->loaded;
}

static void check_progress(voe_ecs_world *world, voe_render_device *device,
			   voe_base_arena *scratch)
{
	voe_3d_models *models = voe_3d_models_new();
	voe_game_progress progress = { 0 };
	voe_game_models_failures failures;
	uint64_t stamp = 0;

	// ---- with a progress: one file counted, and done at the end
	wear(world, GOOD);
	wear(world, GOOD);
	failures = voe_game_models_update(world, models, device, FOLDER, scratch,
					  &progress);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(loaded_at(models, &stamp));
	VOE_TEST_CHECK_INT(atomic_load(&progress.total), 1);
	VOE_TEST_CHECK_INT(atomic_load(&progress.done), 1);
	voe_3d_models_clear(models, device);

	// ---- stop already asked: nothing read; a later call reads it
	atomic_store(&progress.stop, true);
	failures = voe_game_models_update(world, models, device, FOLDER, scratch,
					  &progress);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK_INT(voe_3d_models_count(models), 0);
	failures = voe_game_models_update(world, models, device, FOLDER, scratch,
					  NULL);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(loaded_at(models, &stamp));

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

// The table's landscape held, loaded at stamp 0, heights in metres.
static void check_hill(const voe_3d_models *models)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(models, LANDSCAPE);

	VOE_TEST_CHECK(entry != NULL && entry->loaded &&
		       entry->landscape != NULL);
	if (entry == NULL || entry->landscape == NULL)
		return;
	VOE_TEST_CHECK_INT(entry->stamp, 0);
	VOE_TEST_CHECK_INT(entry->part_count, 1);
	VOE_TEST_CHECK_INT(entry->landscape->cells, 4);
	VOE_TEST_CHECK(entry->landscape->size == 64.0f);
	VOE_TEST_CHECK(entry->landscape->heights[0] == -0.25f);
	VOE_TEST_CHECK(entry->landscape->heights[12] == 1.5f);
	VOE_TEST_CHECK(entry->landscape->heights[24] == 0.0f);
}

static void landscapes_load_from_a_table(voe_3d_models *models,
					 voe_render_device *device,
					 voe_base_arena *scratch)
{
	voe_game_models_failures failures =
		voe_game_models_landscapes(models, device, &HILLS, scratch);

	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(failures.first == NULL);
	check_hill(models);
}

// A text file at the landscape's path would fail a watch that read it.
static void watch_leaves_a_landscape_alone(voe_3d_models *models,
					   voe_render_device *device,
					   voe_base_arena *scratch)
{
	voe_game_models_failures failures;

	VOE_TEST_CHECK(voe_platform_file_write(LANDSCAPE_ON_DISK, NOT_A_GLB,
					       sizeof(NOT_A_GLB), NULL));
	failures = voe_game_models_watch(models, device, FOLDER, scratch);
	VOE_TEST_CHECK_INT(failures.count, 0);
	check_hill(models);
}

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

// Each placed copy scaled `base` times its own scale, on every axis.
static void check_placed_scale(const voe_3d_models *models,
			       const voe_3d_model_entry *entry, float base)
{
	const voe_scene_transform *placed =
		voe_3d_models_landscape_placed(models, entry);

	VOE_TEST_CHECK(placed != NULL);
	for (uint32_t i = 0; placed != NULL && i < 2; i++) {
		const float want = base * (float)COPIES[i].scale * 0.001f;

		VOE_TEST_CHECK(fabsf(placed[i].scale.x - want) < 1e-4f &&
			       fabsf(placed[i].scale.y - want) < 1e-4f &&
			       fabsf(placed[i].scale.z - want) < 1e-4f);
	}
}

// The forest's grid: both copies as cooked, field by field, and placed at
// `base` times their own scale.
static void check_forest(const voe_3d_models *models, float base)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(models, LANDSCAPE);

	VOE_TEST_CHECK(entry != NULL && entry->landscape != NULL);
	if (entry == NULL || entry->landscape == NULL)
		return;
	VOE_TEST_CHECK_INT(entry->landscape->kind_count, 1);
	VOE_TEST_CHECK(strcmp(entry->landscape->kinds[0], GOOD) == 0);
	VOE_TEST_CHECK_INT(entry->landscape->copy_count, 2);
	for (uint32_t i = 0; i < 2 && entry->landscape->copy_count == 2; i++) {
		const voe_assets_landscape_copy *copy = &entry->landscape->copies[i];

		VOE_TEST_CHECK(copy->x == COPIES[i].x && copy->z == COPIES[i].z &&
			       copy->turn == COPIES[i].turn &&
			       copy->scale == COPIES[i].scale &&
			       copy->kind == COPIES[i].kind &&
			       copy->lean == COPIES[i].lean);
	}
	if (entry->landscape->copy_count == 2)
		check_placed_scale(models, entry, base);
}

// The meadow's grid: its kind's look field by field, layer 0 growing nothing
// and layer 1 the kind at 40 tenths.
static void check_meadow(const voe_3d_models *models)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(models, LANDSCAPE);
	const voe_assets_landscape_look *look;

	VOE_TEST_CHECK(entry != NULL && entry->landscape != NULL);
	if (entry == NULL || entry->landscape == NULL)
		return;
	look = &entry->landscape->looks[0];
	VOE_TEST_CHECK_INT(entry->landscape->layer_count, 2);
	VOE_TEST_CHECK_INT(entry->landscape->kind_count, 1);
	VOE_TEST_CHECK(look->size == MEADOW_LOOK.size &&
		       look->size_vary == MEADOW_LOOK.size_vary &&
		       look->turn_vary == MEADOW_LOOK.turn_vary &&
		       look->tilt == MEADOW_LOOK.tilt && look->shadowless);
	VOE_TEST_CHECK_INT(entry->landscape->grows[0][0], 0);
	VOE_TEST_CHECK_INT(entry->landscape->grows[1][0], 40);
}

static void check_foliage(voe_ecs_world *world, voe_render_device *device,
			  voe_base_arena *scratch)
{
	voe_3d_models *models = voe_3d_models_new();
	const voe_3d_model_entry *kind;
	voe_game_models_failures failures;

	// ---- a kind naming the `.glb` at 2×: loaded, both copies held, placed
	failures = voe_game_models_landscapes(models, device, &FOREST, scratch);
	VOE_TEST_CHECK_INT(failures.count, 0);
	failures = voe_game_models_update(world, models, device, FOLDER, scratch,
					  NULL);
	VOE_TEST_CHECK_INT(failures.count, 0);
	kind = voe_3d_models_find(models, GOOD);
	VOE_TEST_CHECK(kind != NULL && kind->loaded);
	check_forest(models, 2.0f);
	voe_3d_models_clear(models, device);

	// ---- no looks: each copy at its own scale
	failures = voe_game_models_landscapes(models, device, &GROVE, scratch);
	VOE_TEST_CHECK_INT(failures.count, 0);
	check_forest(models, 1.0f);
	voe_3d_models_clear(models, device);

	// ---- a look and layer 1's growth: on the grid as cooked
	failures = voe_game_models_landscapes(models, device, &MEADOW, scratch);
	VOE_TEST_CHECK_INT(failures.count, 0);
	check_meadow(models);
	voe_3d_models_clear(models, device);

	// ---- a kind naming a missing file: one failure, named
	failures = voe_game_models_landscapes(models, device, &THICKET, scratch);
	VOE_TEST_CHECK_INT(failures.count, 0);
	failures = voe_game_models_update(world, models, device, FOLDER, scratch,
					  NULL);
	VOE_TEST_CHECK_INT(failures.count, 1);
	VOE_TEST_CHECK(failures.first != NULL &&
		       strcmp(failures.first, MISSING) == 0);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

// A row naming a `.landscape` on disk whose one kind is the `.glb`, as the
// editor's world names its landscape: the kind is no row and no table's.
static void check_row_landscape(voe_ecs_world *world,
				voe_render_device *device,
				voe_base_arena *scratch)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	voe_assets_landscape land = voe_assets_landscape_flat(64.0f, 4, scratch);
	voe_3d_models *models = voe_3d_models_new();
	voe_game_progress progress = { 0 };
	voe_game_models_failures failures;
	voe_assets_landscape_text text;
	const voe_3d_model_entry *kind;

	snprintf(land.kinds[0], sizeof(land.kinds[0]), "%s", GOOD);
	land.kind_count = 1;
	text = voe_assets_landscape_write(&land, scratch);
	VOE_TEST_CHECK(voe_platform_file_write(ROW_LANDSCAPE_ON_DISK,
					       (const uint8_t *)text.text,
					       text.size, NULL));
	voe_base_arena_rewind(scratch, mark);

	// ---- one update with a progress: the kind loaded, and counted
	wear(world, ROW_LANDSCAPE);
	failures = voe_game_models_update(world, models, device, FOLDER, scratch,
					  &progress);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(voe_3d_models_find(models, ROW_LANDSCAPE) != NULL);
	kind = voe_3d_models_find(models, GOOD);
	VOE_TEST_CHECK(kind != NULL && kind->loaded);
	VOE_TEST_CHECK_INT(atomic_load(&progress.total), 2);
	VOE_TEST_CHECK_INT(atomic_load(&progress.done), 2);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

// How many of the store's entries the watch gathers, into a batch with room
// for all of them, looking at every one.
static uint32_t watch_gathered(const voe_3d_models *models,
			       voe_render_device *device,
			       voe_base_arena *scratch)
{
	uint32_t count = voe_3d_models_count(models);
	voe_game_models_batch *batch = voe_game_models_batch_new(
		FOLDER, count, VOE_GAME_MODELS_COOK_NEVER, device, NULL, NULL);
	uint32_t gathered;

	VOE_TEST_CHECK_INT(voe_game_models_watch_gather(models, FOLDER, scratch,
							0, count, batch),
			   count);
	gathered = voe_game_models_batch_count(batch);
	voe_game_models_batch_destroy(batch);
	return gathered;
}

static void check_files(voe_ecs_world *world, voe_render_device *device,
			voe_base_arena *scratch, const uint8_t *glb,
			uint32_t size)
{
	voe_3d_models *models = voe_3d_models_new();
	voe_game_models_failures failures;
	uint64_t first = 0, stamp = 0;

	// ---- a row naming the file: no failure, a loaded entry
	wear(world, GOOD);
	failures = voe_game_models_update(world, models, device, FOLDER, scratch,
					  NULL);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(failures.first == NULL);
	VOE_TEST_CHECK(loaded_at(models, &first));
	VOE_TEST_CHECK(voe_3d_models_find(models, "") == NULL);

	// ---- a row naming a missing file: one failure, named
	wear(world, MISSING);
	failures = voe_game_models_update(world, models, device, FOLDER, scratch,
					  NULL);
	VOE_TEST_CHECK_INT(failures.count, 1);
	VOE_TEST_CHECK(failures.first != NULL &&
		       strcmp(failures.first, MISSING) == 0);
	VOE_TEST_CHECK_INT(voe_3d_models_count(models), 2);

	// ---- nothing changed: the watch gathers nothing, so prepares no batch
	VOE_TEST_CHECK_INT(watch_gathered(models, device, scratch), 0);

	// ---- the file rewritten as text: gathered alone; the watch fails it,
	// still loaded
	VOE_TEST_CHECK(voe_platform_file_write(GOOD_ON_DISK, NOT_A_GLB,
					       sizeof(NOT_A_GLB), NULL));
	VOE_TEST_CHECK_INT(watch_gathered(models, device, scratch), 1);
	failures = voe_game_models_watch(models, device, FOLDER, scratch);
	VOE_TEST_CHECK_INT(failures.count, 1);
	VOE_TEST_CHECK(failures.first != NULL &&
		       strcmp(failures.first, GOOD) == 0);
	VOE_TEST_CHECK(loaded_at(models, &stamp));
	VOE_TEST_CHECK(stamp != first);

	// ---- rewritten valid: the watch loads it again
	VOE_TEST_CHECK(voe_platform_file_write(GOOD_ON_DISK, glb, size, NULL));
	failures = voe_game_models_watch(models, device, FOLDER, scratch);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(loaded_at(models, &first));
	VOE_TEST_CHECK(first != stamp);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

// Every file and folder the cases write, deepest first.
static void remove_files(void)
{
	remove(GOOD_ON_DISK);
	remove(PICTURE_ON_DISK);
	remove(BROKEN_ON_DISK);
	remove(LANDSCAPE_ON_DISK);
	remove(ROW_LANDSCAPE_ON_DISK);
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
	voe_3d_models *landscapes;
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
	check_files(voe_game_world_new(arena), device, scratch, glb, length);
	check_pictures(voe_game_world_new(arena), device, scratch);
	check_water(voe_game_world_new(arena), device, scratch);
	check_progress(voe_game_world_new(arena), device, scratch);
	landscapes = voe_3d_models_new();
	landscapes_load_from_a_table(landscapes, device, scratch);
	watch_leaves_a_landscape_alone(landscapes, device, scratch);
	voe_3d_models_clear(landscapes, device);
	voe_3d_models_destroy(landscapes);
	check_materials(voe_game_world_new(arena), device, scratch);
	broken_load(device, scratch);
	check_layers(voe_game_world_new(arena), device, scratch);
	check_parts(voe_game_world_new(arena), device, scratch);
	check_foliage(voe_game_world_new(arena), device, scratch);
	check_row_landscape(voe_game_world_new(arena), device, scratch);
	remove_files();
	voe_render_device_destroy(device);
released:
	voe_base_arena_destroy(scratch);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
