// The model loader's landscapes against real files: the one-triangle `.glb`
// of glb_build.h written into a scratch folder in the working directory (the
// build tree), cooked landscape tables, and game worlds whose model rows name
// a `.landscape` on disk.
//
// THE LANDSCAPES, on a first store: a 4-cell table loads at stamp 0 with its
// millimetres as metres, one part; with a text file at its path a watch
// reports nothing and leaves it loaded at 0.
//
// THE FOLIAGE, on a second store and an empty world: a landscape naming the
// `.glb` as its one kind, at a base size of 2×, with two copies, makes an
// update load the kind and the grid hold both copies as cooked, placed at
// twice their own scale; with no looks, at their own; two layers, the kind's
// look with spreads and casting none and layer 1 growing it, load with that look
// and those grows on the grid; a kind naming a missing file is one failure, named.
//
// THE ROW'S LANDSCAPE, on a third world and store: a model row naming a
// `.landscape` on disk whose one kind is the `.glb` makes one update with a
// progress load the kind too, total and done 2.
//
// The rows, pictures, water and progress cases are tests/models.c; the
// materials, layers and parts cases tests/models_materials.c.
//
// The files and the folder are removed at the end, pass or fail. It skips
// when there is no graphics card, because a load uploads.
#include "glb_build.h"

#include <game/models.h>
#include <game/progress.h>
#include <game/world.h>

#include <3d/models.h>
#include <3d/models_foliage.h>

#include <assets/landscape.h>

#include <base/arena.h>
#include <base/error.h>

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

#define FOLDER "game_models_landscapes_test"
#define GOOD "good.glb"
#define MISSING "missing.glb"
#define GOOD_ON_DISK FOLDER "/" GOOD
#define LANDSCAPE "hill.landscape"
#define LANDSCAPE_ON_DISK FOLDER "/" LANDSCAPE
#define ROW_LANDSCAPE "forest.landscape"
#define ROW_LANDSCAPE_ON_DISK FOLDER "/" ROW_LANDSCAPE
#define MATERIAL "Assets/m.material"
#define GREY_MATERIAL "Assets/grey.material"
#define CACHE_ON_DISK FOLDER "/Cache"
#define LEVELS_FOLDER_ON_DISK CACHE_ON_DISK "/levels"
#define LEVELS_ON_DISK LEVELS_FOLDER_ON_DISK "/" GOOD ".levels"
#define COPIES_FOLDER_ON_DISK CACHE_ON_DISK "/models"
#define COPY_ON_DISK COPIES_FOLDER_ON_DISK "/" GOOD ".model"

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

// One plane of a 4-cell landscape's paint: all layer 0 but the middle point,
// 200 of layer 1. Filled by main, since a literal would be 100 bytes.
#define PAINTED 12
static uint8_t paint[25 * 4];

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

static const uint8_t NOT_A_GLB[] = "this is not a model";

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

// Every file and folder the cases write, deepest first.
static void remove_files(void)
{
	remove(GOOD_ON_DISK);
	remove(LANDSCAPE_ON_DISK);
	remove(ROW_LANDSCAPE_ON_DISK);
	remove(LEVELS_ON_DISK);
	remove(LEVELS_FOLDER_ON_DISK);
	remove(COPY_ON_DISK);
	remove(COPIES_FOLDER_ON_DISK);
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
	for (uint32_t i = 0; i < 25; i++)
		paint[i * 4] = 255;
	paint[PAINTED * 4] = 55;
	paint[PAINTED * 4 + 1] = 200;
	landscapes = voe_3d_models_new();
	landscapes_load_from_a_table(landscapes, device, scratch);
	watch_leaves_a_landscape_alone(landscapes, device, scratch);
	voe_3d_models_clear(landscapes, device);
	voe_3d_models_destroy(landscapes);
	check_foliage(voe_game_world_new(arena), device, scratch);
	check_row_landscape(voe_game_world_new(arena), device, scratch);
	remove_files();
	voe_render_device_destroy(device);
released:
	voe_base_arena_destroy(scratch);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
