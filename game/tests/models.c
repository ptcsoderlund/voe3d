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
// The landscapes, foliage and row's landscape cases are
// tests/models_landscapes.c; the materials, broken map, layers and parts
// cases tests/models_materials.c; the levels cases tests/models_levels.c; the
// `.glb` and the world helpers they all use are tests/glb_build.h.
//
// The files and the folder are removed at the end, pass or fail. It skips
// when there is no graphics card, because a load uploads.
#include "glb_build.h"

#include "../src/models_batch.h"

#include <game/models.h>
#include <game/progress.h>
#include <game/world.h>

#include <3d/emitter_component.h>
#include <3d/models.h>
#include <3d/water_component.h>

#include <base/arena.h>
#include <base/error.h>

#include <platform/file.h>
#include <platform/folder.h>

#include <render/device.h>

#include <testing/test.h>

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
	check_files(voe_game_world_new(arena), device, scratch, glb, length);
	check_pictures(voe_game_world_new(arena), device, scratch);
	check_water(voe_game_world_new(arena), device, scratch);
	check_progress(voe_game_world_new(arena), device, scratch);
	remove_files();
	voe_render_device_destroy(device);
released:
	voe_base_arena_destroy(scratch);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
