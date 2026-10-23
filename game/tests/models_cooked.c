// The model loader's cooked copies against real files: the one-triangle
// `.glb` of glb_build.h written into a scratch folder in the working
// directory (the build tree), and a game world whose model row names it. Each
// case loads into a store of its own.
//
// update_writes_the_copy: an update leaves `Cache/models/good.glb.model`,
// keyed by the `.glb`'s stamp and 0 for no `.parts`.
// a_fresh_copy_is_read_not_written_again: a second update loads, the copy's
// stamp unchanged.
// a_changed_glb_writes_it_again: the `.glb` rewritten a size longer, an update
// writes the copy keyed by its new stamp.
// watch_reloads_and_writes_no_copy: the copy gone and the `.glb` changed, the
// watch reloads it at its new stamp and no copy is there.
// update_cooked_loads_a_copy_whose_glb_is_garbage: a copy written, then the
// `.glb` made bytes no `.glb` has, the game's update loads it from the copy,
// which keeps its old key.
// prepare_writes_the_copy: with the levels file fresh and the copy gone, a
// prepare makes it, keyed by the stamp; a second prepare makes nothing.
//
// The files and the folders are removed at the end, pass or fail. It skips
// when there is no graphics card, because a load uploads.
#include "glb_build.h"

#include <game/models.h>
#include <game/world.h>

#include <3d/model_cooked.h>
#include <3d/model_levels.h>
#include <3d/models.h>

#include <base/arena.h>
#include <base/error.h>

#include <platform/file.h>
#include <platform/folder.h>

#include <render/device.h>

#include <testing/test.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define FOLDER "game_models_cooked_test"
#define GOOD "good.glb"
#define GOOD_ON_DISK FOLDER "/" GOOD
#define CACHE_ON_DISK FOLDER "/Cache"
#define MODELS_FOLDER_ON_DISK CACHE_ON_DISK "/models"
#define COPY_ON_DISK MODELS_FOLDER_ON_DISK "/" GOOD ".model"
#define LEVELS_FOLDER_ON_DISK CACHE_ON_DISK "/levels"
#define LEVELS_ON_DISK LEVELS_FOLDER_ON_DISK "/" GOOD ".levels"

// Room for the triangle twice over a watch's reload, with its levels, and the
// shadings of its part and a blended twin, each twice.
static const voe_render_capacities CAPACITIES = {
	.vertices = 64,
	.indices = 64,
	.geometries = VOE_3D_MODEL_LEVELS * 2,
	.objects = 1,
	.shadings = 16,
	.passes = 1,
};

// The one-triangle `.glb` written, its JSON `pad` spaces longer when not 0,
// so its size and stamp differ; its stamp returned.
static uint64_t glb_write(uint32_t pad)
{
	static char text[sizeof(JSON) + 16];
	uint8_t glb[512];
	uint32_t length;
	uint64_t stamp = 0;

	VOE_TEST_CHECK(pad < 16);
	snprintf(text, sizeof(text), "%s%*s", JSON, (int)pad, "");
	length = glb_pack(glb, sizeof(glb), text, TRIANGLE,
			  (uint32_t)sizeof(TRIANGLE));
	VOE_TEST_CHECK(voe_platform_file_write(GOOD_ON_DISK, glb, length, NULL));
	VOE_TEST_CHECK(voe_platform_file_stamp(GOOD_ON_DISK, &stamp));
	return stamp;
}

// The copy's key from its head; zeroed when there is none or it will not read.
static voe_3d_model_cooked_key copy_key(void)
{
	uint8_t head[VOE_3D_MODEL_COOKED_HEAD];
	voe_3d_model_cooked_key key = { 0 };
	size_t count = 0;

	if (!voe_platform_file_read_head(COPY_ON_DISK, head, sizeof(head),
					 &count, NULL) ||
	    !voe_3d_model_cooked_key_of(head, count, &key))
		return (voe_3d_model_cooked_key){ 0 };
	return key;
}

// The copy's stamp, 0 when there is none.
static uint64_t copy_stamp(void)
{
	uint64_t stamp = 0;

	return voe_platform_file_stamp(COPY_ON_DISK, &stamp) ? stamp : 0;
}

// A new store the world is updated into, the game's way when `cooked`, with
// no failure and the `.glb` loaded at `stamp`.
static voe_3d_models *store_updated(const voe_ecs_world *world,
				    voe_render_device *device,
				    voe_base_arena *scratch, bool cooked,
				    uint64_t stamp)
{
	voe_3d_models *models = voe_3d_models_new();
	voe_game_models_failures failures =
		cooked ? voe_game_models_update_cooked(world, models, device,
						       FOLDER, scratch, NULL) :
			 voe_game_models_update(world, models, device, FOLDER,
						scratch, NULL);
	const voe_3d_model_entry *entry = voe_3d_models_find(models, GOOD);

	VOE_TEST_CHECK_INT(failures.count, 0);
	VOE_TEST_CHECK(entry != NULL && entry->loaded && entry->stamp == stamp);
	return models;
}

static void store_gone(voe_3d_models *models, voe_render_device *device)
{
	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

static void update_writes_the_copy(const voe_ecs_world *world,
				   voe_render_device *device,
				   voe_base_arena *scratch)
{
	uint64_t stamp = glb_write(0);
	voe_3d_models *models;

	remove(COPY_ON_DISK);
	models = store_updated(world, device, scratch, false, stamp);
	VOE_TEST_CHECK(copy_key().glb == stamp);
	VOE_TEST_CHECK(copy_key().parts == 0);
	store_gone(models, device);
}

static void a_fresh_copy_is_read_not_written_again(const voe_ecs_world *world,
						   voe_render_device *device,
						   voe_base_arena *scratch)
{
	uint64_t stamp = 0;
	uint64_t before = copy_stamp();
	voe_3d_models *models;

	VOE_TEST_CHECK(before != 0);
	VOE_TEST_CHECK(voe_platform_file_stamp(GOOD_ON_DISK, &stamp));
	models = store_updated(world, device, scratch, false, stamp);
	VOE_TEST_CHECK(copy_stamp() == before);
	store_gone(models, device);
}

static void a_changed_glb_writes_it_again(const voe_ecs_world *world,
					  voe_render_device *device,
					  voe_base_arena *scratch)
{
	uint64_t before = copy_key().glb;
	uint64_t stamp = glb_write(4);
	voe_3d_models *models;

	VOE_TEST_CHECK(stamp != before);
	models = store_updated(world, device, scratch, false, stamp);
	VOE_TEST_CHECK(copy_key().glb == stamp);
	store_gone(models, device);
}

static void watch_reloads_and_writes_no_copy(const voe_ecs_world *world,
					     voe_render_device *device,
					     voe_base_arena *scratch)
{
	uint64_t stamp = 0;
	voe_3d_models *models;
	voe_game_models_failures failures;
	const voe_3d_model_entry *entry;

	VOE_TEST_CHECK(voe_platform_file_stamp(GOOD_ON_DISK, &stamp));
	models = store_updated(world, device, scratch, false, stamp);
	remove(COPY_ON_DISK);
	stamp = glb_write(8);
	failures = voe_game_models_watch(models, device, FOLDER, scratch);
	VOE_TEST_CHECK_INT(failures.count, 0);
	entry = voe_3d_models_find(models, GOOD);
	VOE_TEST_CHECK(entry != NULL && entry->loaded && entry->stamp == stamp);
	VOE_TEST_CHECK(!voe_platform_file_exists(COPY_ON_DISK));
	store_gone(models, device);
}

static void
update_cooked_loads_a_copy_whose_glb_is_garbage(const voe_ecs_world *world,
						voe_render_device *device,
						voe_base_arena *scratch)
{
	static const uint8_t GARBAGE[] = "these bytes are no glb";
	uint64_t stamp = glb_write(0);
	uint64_t garbage = 0;
	voe_3d_models *models;

	// ---- a copy written by an update
	remove(COPY_ON_DISK);
	store_gone(store_updated(world, device, scratch, false, stamp), device);
	VOE_TEST_CHECK(copy_key().glb == stamp);

	// ---- the `.glb` garbage: the game's update loads the copy, kept
	VOE_TEST_CHECK(voe_platform_file_write(GOOD_ON_DISK, GARBAGE,
					       sizeof(GARBAGE) - 1, NULL));
	VOE_TEST_CHECK(voe_platform_file_stamp(GOOD_ON_DISK, &garbage));
	models = store_updated(world, device, scratch, true, garbage);
	VOE_TEST_CHECK(copy_key().glb == stamp);
	store_gone(models, device);
}

static void prepare_writes_the_copy(voe_base_arena *scratch)
{
	uint64_t stamp = glb_write(0);
	voe_base_error error = VOE_BASE_OK;
	bool made = false;

	// ---- a first prepare leaves both files fresh
	VOE_TEST_CHECK(voe_game_models_prepare(FOLDER, GOOD, scratch, false,
					       &made, &error));
	VOE_TEST_CHECK(voe_platform_file_exists(LEVELS_ON_DISK));

	// ---- the copy gone: made again, keyed by the stamp
	remove(COPY_ON_DISK);
	made = false;
	VOE_TEST_CHECK(voe_game_models_prepare(FOLDER, GOOD, scratch, false,
					       &made, &error));
	VOE_TEST_CHECK(made);
	VOE_TEST_CHECK(copy_key().glb == stamp);

	// ---- both fresh: nothing made
	VOE_TEST_CHECK(voe_game_models_prepare(FOLDER, GOOD, scratch, false,
					       &made, &error));
	VOE_TEST_CHECK(!made);
}

// Every file and folder the cases write, deepest first.
static void remove_files(void)
{
	remove(GOOD_ON_DISK);
	remove(COPY_ON_DISK);
	remove(MODELS_FOLDER_ON_DISK);
	remove(LEVELS_ON_DISK);
	remove(LEVELS_FOLDER_ON_DISK);
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
	voe_ecs_world *world;

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
	world = voe_game_world_new(arena);
	wear(world, GOOD);
	update_writes_the_copy(world, device, scratch);
	a_fresh_copy_is_read_not_written_again(world, device, scratch);
	a_changed_glb_writes_it_again(world, device, scratch);
	watch_reloads_and_writes_no_copy(world, device, scratch);
	update_cooked_loads_a_copy_whose_glb_is_garbage(world, device, scratch);
	prepare_writes_the_copy(scratch);
	remove_files();
	voe_render_device_destroy(device);
released:
	voe_base_arena_destroy(scratch);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
