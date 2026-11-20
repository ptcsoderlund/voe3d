// game/trees.h on a headless device: one species of three variations, grown
// from the default values, into a store, then the same row by a job; and the
// loader leaving a tree's path to them.
//
// THE CASES, each on the store cleared after the one before:
// _grow gives no failure, a species entry `a.tree` holding three variations,
// and each `a.tree#v` loaded and grown with four levels; a job started on the
// row is busy until put whole, done after polling, and puts in three calls,
// the third setting the same species; a landscape whose one kind is `a.tree`
// loaded and an update with a progress over an empty world counts nothing to
// read, fails nothing and leaves `a.tree` unasked in the store.
//
// The folder the update names is never made: a tree's path is never read.
// It skips when there is no graphics card, because a put uploads.
#include <game/models.h>
#include <game/progress.h>
#include <game/trees.h>
#include <game/world.h>

#include <3d/models.h>
#include <3d/tree_grow.h>

#include <assets/tree.h>

#include <base/arena.h>
#include <base/error.h>

#include <render/device.h>

#include <testing/test.h>

#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <threads.h>

#define SPECIES "a.tree"
// Polls of a millisecond a job may take to grow three trees.
#define POLLS 60000

// Three variations of four levels, two primitives each, held at once, and a
// landscape's grid.
static const voe_render_capacities ROOM = {
	.vertices = 1 << 18,
	.indices = 1 << 19,
	.geometries = 64,
	.objects = 8,
	.shadings = 64,
	.passes = 1,
};

// A flat 4-cell landscape whose one kind is the species.
static const int32_t FLAT[25];

// The species entry holds three loaded, grown variations of four levels.
static void check_species(const voe_3d_models *models)
{
	const voe_3d_model_entry *species = voe_3d_models_find(models, SPECIES);

	VOE_TEST_CHECK(species != NULL);
	if (species == NULL)
		return;
	VOE_TEST_CHECK(species->loaded && species->part_count == 0);
	VOE_TEST_CHECK_INT(species->variation_count, 3);
	for (uint32_t v = 0; v < species->variation_count; v++) {
		const voe_3d_model_entry *variation;

		VOE_TEST_CHECK(species->variations[v] < voe_3d_models_count(models));
		if (species->variations[v] >= voe_3d_models_count(models))
			continue;
		variation = voe_3d_models_at(models, species->variations[v]);
		VOE_TEST_CHECK(variation->loaded && variation->grown);
		VOE_TEST_CHECK_INT(variation->level_count, VOE_3D_TREE_LEVELS);
	}
}

static void grows_a_species(voe_3d_models *models, voe_render_device *device,
			    const voe_game_tree *row, voe_base_arena *scratch)
{
	const voe_game_trees trees = { row, 1 };
	voe_game_progress progress = { 0 };
	voe_game_models_failures failures;

	failures = voe_game_trees_grow(models, device, &trees, scratch,
				       &progress);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK_INT(atomic_load(&progress.total), 3);
	VOE_TEST_CHECK_INT(atomic_load(&progress.done), 3);
	check_species(models);
	voe_3d_models_clear(models, device);
}

static void a_job_puts_a_variation_a_call(voe_3d_models *models,
					  voe_render_device *device,
					  const voe_game_tree *row)
{
	const struct timespec millisecond = { .tv_nsec = 1000000 };
	voe_game_trees_job *job = voe_game_trees_job_new();
	voe_base_error error = VOE_BASE_OK;
	uint32_t polls = 0;

	VOE_TEST_CHECK(!voe_game_trees_job_done(job));
	VOE_TEST_CHECK(voe_game_trees_job_start(job, row));
	VOE_TEST_CHECK(!voe_game_trees_job_start(job, row));
	while (!voe_game_trees_job_done(job) && polls++ < POLLS)
		(void)thrd_sleep(&millisecond, NULL);
	VOE_TEST_CHECK(voe_game_trees_job_done(job));
	if (voe_game_trees_job_done(job)) {
		VOE_TEST_CHECK(!voe_game_trees_job_put(job, models, device,
						       &error));
		VOE_TEST_CHECK(!voe_game_trees_job_put(job, models, device,
						       &error));
		VOE_TEST_CHECK(voe_game_trees_job_put(job, models, device,
						      &error));
		VOE_TEST_CHECK_INT(error, VOE_BASE_OK);
		VOE_TEST_CHECK(!voe_game_trees_job_done(job));
		check_species(models);
	}
	voe_game_trees_job_destroy(job);
	voe_3d_models_clear(models, device);
}

static void an_update_reads_no_tree(voe_3d_models *models,
				    voe_render_device *device,
				    voe_ecs_world *world,
				    voe_base_arena *scratch)
{
	const voe_game_landscape land = {
		"a.landscape", 64.0f, 4, FLAT, NULL, 0, NULL,
		(const char *const[]){ SPECIES }, 1, NULL, 0, NULL, NULL
	};
	const voe_game_landscapes landscapes = { &land, 1 };
	voe_game_progress progress = { 0 };
	voe_game_models_failures failures;

	failures = voe_game_models_landscapes(models, device, &landscapes,
					      scratch);
	VOE_TEST_CHECK_INT(failures.count, 0);
	failures = voe_game_models_update(world, models, device,
					  "game_trees_test", scratch,
					  &progress);
	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK_INT(atomic_load(&progress.total), 0);
	VOE_TEST_CHECK(voe_3d_models_find(models, SPECIES) == NULL);
	voe_3d_models_clear(models, device);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1 << 20);
	voe_base_arena *scratch = voe_base_arena_new(1 << 16);
	voe_platform_size size = { 16, 16 };
	voe_base_error error = VOE_BASE_OK;
	voe_game_tree row = { SPECIES, voe_assets_tree_default() };
	voe_render_device *device;
	voe_3d_models *models;

	row.values.variations = 3;
	device = voe_render_device_new_headless(arena, size, ROOM, &error);
	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(device != NULL);
		goto released;
	}
	models = voe_3d_models_new();
	grows_a_species(models, device, &row, scratch);
	a_job_puts_a_variation_a_call(models, device, &row);
	an_update_reads_no_tree(models, device, voe_game_world_new(arena),
				scratch);
	voe_3d_models_destroy(models);
	voe_render_device_destroy(device);
released:
	voe_base_arena_destroy(scratch);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
