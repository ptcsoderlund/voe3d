// The model loader beside the frames on real files: the one-triangle `.glb`
// of tests/glb_build.h written into a scratch folder in the working directory
// (the build tree), a game world whose model row names it, and a store with a
// pool of 3 threads.
//
// THE CASES, each on a loader of its own: the first step leaves the store
// without the file; steps, a yield between, load it within 30 s with no
// failure. The file re-written, steps with `watch` load it again at its new
// stamp. On a cleared store, a step for the folder puts a batch out, and
// steps for an empty second folder drop it: the entry ends failed with one
// failure, named, as the second folder lacks the file, never loaded from the
// first.
// Last, a fresh loader destroyed while its first prepare runs returns, the
// store untouched.
//
// The files and folders are removed at the end, pass or fail. It skips when
// there is no graphics card, because a load uploads.
#include "glb_build.h"

#include <game/models_loader.h>
#include <game/world.h>

#include <3d/models.h>

#include <base/arena.h>
#include <base/error.h>
#include <base/workers.h>

#include <platform/clock.h>
#include <platform/file.h>
#include <platform/folder.h>

#include <render/device.h>

#include <testing/test.h>

#include <stdio.h>
#include <string.h>
#include <threads.h>

#define FOLDER "game_models_loader_test"
#define EMPTY "game_models_loader_empty"
#define GOOD "good.glb"
#define GOOD_ON_DISK FOLDER "/" GOOD
#define CACHE_ON_DISK FOLDER "/Cache"
#define LEVELS_FOLDER_ON_DISK CACHE_ON_DISK "/levels"
#define LEVELS_ON_DISK LEVELS_FOLDER_ON_DISK "/" GOOD ".levels"

// How long a case steps before it gives up, in seconds.
#define PATIENCE 30.0

// Room for the triangle's levels twice over a reload and the shadings the
// model tests give it.
static const voe_render_capacities CAPACITIES = {
	.vertices = 16,
	.indices = 16,
	.geometries = VOE_3D_MODEL_LEVELS * 2,
	.objects = 1,
	.shadings = 10 + 2 + 2 + 2,
	.passes = 1,
};

// What every step of a case is handed.
struct stepping {
	voe_game_models_loader *loader;
	const voe_ecs_world *world;
	voe_3d_models *models;
	voe_render_device *device;
	voe_base_arena *scratch;
};

static voe_game_models_failures step(const struct stepping *s,
				     const char *folder, bool watch)
{
	return voe_game_models_loader_step(s->loader, s->world, s->models,
					   s->device, folder, watch, s->scratch);
}

static bool loaded_at(const voe_3d_models *models, uint64_t *stamp)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(models, GOOD);

	if (entry == NULL)
		return false;
	*stamp = entry->stamp;
	return entry->loaded;
}

// Steps for FOLDER, a yield before each, until the file is loaded at `want`
// (any stamp for 0) or PATIENCE runs out; the failures' count returned.
static uint32_t step_until_loaded(const struct stepping *s, bool watch,
				  uint64_t want)
{
	double start = voe_platform_clock_now();
	uint32_t failed = 0;
	uint64_t stamp = 0;

	while ((!loaded_at(s->models, &stamp) || (want != 0 && stamp != want)) &&
	       voe_platform_clock_now() - start < PATIENCE) {
		thrd_yield();
		failed += step(s, FOLDER, watch).count;
	}
	VOE_TEST_CHECK(loaded_at(s->models, &stamp));
	VOE_TEST_CHECK(want == 0 || stamp == want);
	return failed;
}

// The `.glb` written again until its stamp differs from `old`: the new one.
static uint64_t rewrite(const uint8_t *glb, uint32_t size, uint64_t old)
{
	double start = voe_platform_clock_now();
	uint64_t disk = old;

	while (disk == old && voe_platform_clock_now() - start < PATIENCE) {
		VOE_TEST_CHECK(voe_platform_file_write(GOOD_ON_DISK, glb, size,
						       NULL));
		VOE_TEST_CHECK(voe_platform_file_stamp(GOOD_ON_DISK, &disk));
	}
	VOE_TEST_CHECK(disk != old);
	return disk;
}

static void check_load(const struct stepping *s, const uint8_t *glb,
		       uint32_t size)
{
	voe_game_models_failures failures;
	uint64_t first = 0;

	// ---- the first step starts the read; the store is still without it
	failures = step(s, FOLDER, false);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(voe_3d_models_find(s->models, GOOD) == NULL);

	// ---- steps until it is loaded, with no failure
	VOE_TEST_CHECK_INT(step_until_loaded(s, false, 0), 0);
	VOE_TEST_CHECK(loaded_at(s->models, &first));

	// ---- re-written, steps with the watch: loaded at its new stamp
	VOE_TEST_CHECK_INT(step_until_loaded(s, true, rewrite(glb, size, first)),
			   0);
}

static void check_drop(const struct stepping *s)
{
	const voe_3d_model_entry *entry = NULL;
	double start = voe_platform_clock_now();
	uint32_t failed = 0;
	bool named = true;

	// ---- a batch out for the folder
	VOE_TEST_CHECK_INT(step(s, FOLDER, false).count, 0);
	VOE_TEST_CHECK(voe_3d_models_find(s->models, GOOD) == NULL);

	// ---- steps for the empty folder: dropped, then failed there
	while (entry == NULL && voe_platform_clock_now() - start < PATIENCE) {
		voe_game_models_failures failures;

		thrd_yield();
		failures = step(s, EMPTY, false);
		failed += failures.count;
		named = named && (failures.count == 0 ||
				  strcmp(failures.first, GOOD) == 0);
		entry = voe_3d_models_find(s->models, GOOD);
	}
	VOE_TEST_CHECK(entry != NULL && !entry->loaded);
	VOE_TEST_CHECK_INT(failed, 1);
	VOE_TEST_CHECK(named);
}

static void check_destroy(const struct stepping *s)
{
	struct stepping fresh = *s;

	// ---- destroyed while its first prepare runs: returns, store untouched
	fresh.loader = voe_game_models_loader_new();
	VOE_TEST_CHECK_INT(step(&fresh, FOLDER, false).count, 0);
	voe_game_models_loader_destroy(fresh.loader);
	VOE_TEST_CHECK_INT(voe_3d_models_count(s->models), 0);
}

// Every file and folder the cases write, deepest first.
static void remove_files(void)
{
	remove(GOOD_ON_DISK);
	remove(LEVELS_ON_DISK);
	remove(LEVELS_FOLDER_ON_DISK);
	remove(CACHE_ON_DISK);
	remove(FOLDER);
	remove(EMPTY);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1 << 20);
	voe_base_arena *scratch = voe_base_arena_new(1 << 16);
	voe_platform_size size = { 16, 16 };
	voe_base_error error = VOE_BASE_OK;
	voe_base_workers *workers;
	voe_ecs_world *world;
	struct stepping s;
	uint8_t glb[512];
	uint32_t length = glb_build(glb, sizeof(glb));

	s.device = voe_render_device_new_headless(arena, size, CAPACITIES,
						  &error);
	if (s.device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(s.device != NULL);
		goto released;
	}
	// A folder left by a run that crashed goes first.
	remove_files();
	VOE_TEST_CHECK(voe_platform_folder_create(FOLDER, NULL));
	VOE_TEST_CHECK(voe_platform_folder_create(EMPTY, NULL));
	VOE_TEST_CHECK(voe_platform_file_write(GOOD_ON_DISK, glb, length, NULL));
	workers = voe_base_workers_new(3);
	s.loader = voe_game_models_loader_new();
	s.models = voe_3d_models_new();
	s.scratch = scratch;
	voe_3d_models_workers_set(s.models, workers);
	world = voe_game_world_new(arena);
	wear(world, GOOD);
	s.world = world;
	check_load(&s, glb, length);
	// A watch pass may still be out; the drop case starts with none.
	voe_game_models_loader_destroy(s.loader);
	s.loader = voe_game_models_loader_new();
	voe_3d_models_clear(s.models, s.device);
	check_drop(&s);
	voe_3d_models_clear(s.models, s.device);
	check_destroy(&s);
	voe_game_models_loader_destroy(s.loader);
	voe_3d_models_clear(s.models, s.device);
	voe_3d_models_destroy(s.models);
	voe_base_workers_destroy(workers);
	remove_files();
	voe_render_device_destroy(s.device);
released:
	voe_base_arena_destroy(scratch);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
