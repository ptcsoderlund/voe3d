// The loader of game/include/game/models_loader.h: one batch at a time
// (models_batch.h), gathered on the caller, prepared on a `thrd_t` of the
// loader's and put a file a step. The thread does the batch's prepare alone
// and touches nothing else; the caller leaves the batch and the times alone
// until the atomic `prepared` says it is done, then joins it. With no thread
// to be had, the prepare runs on the caller, slow but still a load.
//
// The loader lives in an arena of its own, the batch's folder copied in after
// it, rewound when the batch goes. A gathered batch that found nothing is kept,
// empty, for the next step's gathering, so a quiet frame allocates nothing.
#include <game/models_loader.h>

#include "models_batch.h"

#include <base/assert.h>

#include <stdatomic.h>
#include <string.h>
#include <threads.h>

// The loader and a folder path; a longer folder gets a block of its own.
#define LOADER_BLOCK 4096

struct voe_game_models_loader {
	voe_base_arena *arena;
	// Where the arena stands with the loader alone in it.
	struct voe_base_arena_mark empty;
	// NULL, gathered and not started, or out: started, then prepared.
	voe_game_models_batch *batch;
	// The batch's folder, in the arena, and its room.
	const char *folder;
	uint32_t room;
	bool out;
	// A thread started and not yet joined.
	bool threaded;
	atomic_bool prepared;
	thrd_t thread;
	voe_base_workers *workers;
	// The out batch's next file to put.
	uint32_t next;
	// A watch pass running, and the entry it goes on from.
	bool watching;
	uint32_t watch_next;
	voe_game_models_times times;
};

voe_game_models_loader *voe_game_models_loader_new(void)
{
	voe_base_arena *arena = voe_base_arena_new(LOADER_BLOCK);
	voe_game_models_loader *loader = voe_base_arena_push(arena,
							     sizeof(*loader));

	loader->arena = arena;
	atomic_init(&loader->prepared, false);
	loader->empty = voe_base_arena_mark(arena);
	VOE_BASE_ASSERT(loader->batch == NULL, "a new loader with a batch");
	return loader;
}

// The batch destroyed, its folder and times with it; none is nothing.
static void batch_drop(voe_game_models_loader *loader)
{
	VOE_BASE_ASSERT(!loader->threaded, "dropping a batch preparing");
	if (loader->batch != NULL)
		voe_game_models_batch_destroy(loader->batch);
	loader->batch = NULL;
	loader->folder = NULL;
	loader->out = false;
	loader->times = (voe_game_models_times){ 0 };
	voe_base_arena_rewind(loader->arena, loader->empty);
}

void voe_game_models_loader_destroy(voe_game_models_loader *loader)
{
	VOE_BASE_ASSERT(loader != NULL, "destroying no loader");
	if (loader->threaded)
		(void)thrd_join(loader->thread, NULL);
	loader->threaded = false;
	batch_drop(loader);
	voe_base_arena_destroy(loader->arena);
}

// The thread's whole work: the batch's prepare, then done said.
static int prepare_run(void *argument)
{
	voe_game_models_loader *loader = argument;

	VOE_BASE_ASSERT(loader != NULL && loader->batch != NULL,
			"preparing no batch");
	voe_game_models_batch_prepare(loader->batch, loader->workers,
				      &loader->times);
	atomic_store(&loader->prepared, true);
	return 0;
}

// The gathered batch's prepare started over `workers`.
static void prepare_start(voe_game_models_loader *loader,
			  voe_base_workers *workers)
{
	VOE_BASE_ASSERT(!loader->out && loader->batch != NULL,
			"starting no batch");
	loader->workers = workers;
	loader->next = 0;
	loader->out = true;
	atomic_store(&loader->prepared, false);
	loader->threaded = thrd_create(&loader->thread, prepare_run, loader) ==
			   thrd_success;
	if (!loader->threaded)
		(void)prepare_run(loader);
}

// The prepared batch's held files passed over, then its next file put; after
// the last, or when the store is full, the line and the batch dropped. A
// batch for another folder is dropped whole.
static void put_next(voe_game_models_loader *loader, voe_3d_models *models,
		     voe_render_device *device, const char *folder,
		     voe_game_models_failures *failures)
{
	uint32_t count = voe_game_models_batch_count(loader->batch);
	voe_base_error error = VOE_BASE_OK;
	bool room = true;

	VOE_BASE_ASSERT(loader->out && !loader->threaded, "putting unprepared");
	if (strcmp(loader->folder, folder) != 0) {
		batch_drop(loader);
		return;
	}
	while (loader->next < count &&
	       voe_game_models_batch_held(loader->batch, models, loader->next))
		loader->next++;
	if (loader->next < count)
		room = voe_game_models_batch_put(loader->batch, models, device,
						 loader->next++, failures,
						 &error, &loader->times);
	if (room && loader->next < count)
		return;
	voe_game_models_times_write(&loader->times, "models",
				    VOE_GAME_MODELS_LINE);
	batch_drop(loader);
}

// A batch for `folder` and `room` made unless one waits empty for them.
static void batch_ready(voe_game_models_loader *loader, const char *folder,
			uint32_t room)
{
	size_t length = strlen(folder) + 1;
	char *copy;

	if (loader->batch != NULL &&
	    (loader->room != room || strcmp(loader->folder, folder) != 0))
		batch_drop(loader);
	if (loader->batch != NULL)
		return;
	copy = voe_base_arena_push(loader->arena, length);
	memcpy(copy, folder, length);
	loader->folder = copy;
	loader->room = room;
	loader->batch = voe_game_models_batch_new(copy, room);
}

// The paths an update reads and, in a watch pass, the entries the watch
// reads, up to a chunk, started when there are any.
static void gather_start(voe_game_models_loader *loader,
			 const voe_ecs_world *world, voe_3d_models *models,
			 const char *folder)
{
	voe_base_workers *workers = voe_3d_models_workers(models);
	uint32_t room = voe_base_workers_threads(workers);
	voe_game_models_kinds kinds;

	VOE_BASE_ASSERT(!loader->out, "gathering with a batch out");
	batch_ready(loader, folder, room);
	// A full store ends an update; held entries still reload.
	if (voe_3d_models_count(models) < VOE_3D_MODELS) {
		voe_game_models_kinds_gather(world, models, &kinds);
		(void)voe_game_models_update_gather(world, models, &kinds, 0,
						    room, loader->batch);
	}
	if (loader->watching) {
		loader->watch_next = voe_game_models_watch_gather(
			models, loader->watch_next, room, loader->batch);
		loader->watching =
			loader->watch_next < voe_3d_models_count(models);
	}
	if (voe_game_models_batch_count(loader->batch) > 0)
		prepare_start(loader, workers);
}

voe_game_models_failures voe_game_models_loader_step(
	voe_game_models_loader *loader, const voe_ecs_world *world,
	voe_3d_models *models, voe_render_device *device, const char *folder,
	bool watch, voe_base_arena *scratch)
{
	voe_game_models_failures failures = { 0 };

	VOE_BASE_ASSERT(loader != NULL && models != NULL && device != NULL,
			"no loader, store or device");
	VOE_BASE_ASSERT(folder != NULL && scratch != NULL,
			"no folder or scratch");
	if (watch && !loader->watching) {
		loader->watching = true;
		loader->watch_next = 0;
	}
	if (loader->threaded && atomic_load(&loader->prepared)) {
		(void)thrd_join(loader->thread, NULL);
		loader->threaded = false;
	}
	if (loader->out && !loader->threaded)
		put_next(loader, models, device, folder, &failures);
	if (!loader->out)
		gather_start(loader, world, models, folder);
	voe_game_models_dot_and_water(world, models, device, &failures);
	return failures;
}
