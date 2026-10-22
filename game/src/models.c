// The model loader of game/include/game/models.h: the paths a call loads
// gathered in chunks of the store's pool's threads (0448 point 3), each chunk a
// batch (models_batch.h) prepared in phases over the pool and put file by file
// in order, then the soft dot loaded when any emitter is there and the water
// record when any water is, each failure counted and the times' line written
// once. An update gathers the model rows' paths and the emitters' textures the
// store lacks, each named first there, then every loaded landscape's kind
// paths the store lacks, gathered first; with a progress the distinct unread
// paths are counted first, "Loading models" set before each put, and the stop
// flag asked before each chunk. A full store ends an update before its next
// chunk or put. The watch adds every entry but landscapes and materials at
// its stamp, read only when that changed; a reload adds its one path, forced.
// The update's and the watch's gathering and the dot and water are internal
// calls (models_batch.h).
// A cooked landscape's millimetres become metres in scratch, rewound after it,
// its layer and kind paths copied, its copies handed on for the store to copy,
// its paint widened to both planes in scratch too. The public prepare reads
// its `.glb` and levels file on scratch, rewound; the triangle counts of a
// `.glb`'s levels are read for the LODs panel, nothing made or written.
#include <game/models.h>

#include "models_batch.h"

#include <3d/emitter_component.h>
#include <3d/model_component.h>
#include <3d/model_levels.h>
#include <3d/water_component.h>

#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>

#include <platform/file.h>
#include <platform/path.h>

#include <stddef.h>
#include <string.h>

uint32_t voe_game_models_levels_triangles(const char *folder, const char *path,
					  voe_base_arena *scratch,
					  uint32_t triangles[VOE_3D_MODEL_LEVELS],
					  bool *own, bool *fresh)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	const uint8_t *glb;
	const uint8_t *levels = NULL;
	const char *file;
	size_t glb_size = 0;
	size_t size = 0;
	uint32_t count = 0;

	VOE_BASE_ASSERT(folder != NULL && path != NULL && scratch != NULL,
			"no folder, path or scratch");
	VOE_BASE_ASSERT(triangles != NULL && own != NULL && fresh != NULL,
			"no triangles, own or fresh");
	*own = false;
	*fresh = false;
	voe_base_report_subject_push(path);
	glb = voe_platform_file_read(voe_platform_path_join(scratch, folder,
							    path),
				     scratch, &glb_size, NULL);
	file = voe_game_models_levels_file(
		voe_platform_path_join(scratch, folder, "Cache/levels"), path,
		scratch);
	if (glb != NULL && voe_platform_file_exists(file))
		levels = voe_platform_file_read(file, scratch, &size, NULL);
	if (glb != NULL)
		count = voe_3d_model_levels_triangles(scratch, glb, glb_size,
						      levels, size, triangles,
						      own, fresh);
	voe_base_report_subject_pop();
	voe_base_arena_rewind(scratch, mark);
	return count;
}

bool voe_game_models_prepare(const char *folder, const char *path,
			     voe_base_arena *scratch, voe_base_workers *workers,
			     bool remake, bool *made, voe_base_error *error)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	const uint8_t *glb;
	size_t glb_size = 0;
	size_t size = 0;
	bool ready;

	VOE_BASE_ASSERT(folder != NULL && path != NULL, "no folder or path");
	VOE_BASE_ASSERT(scratch != NULL && made != NULL && error != NULL,
			"no scratch, made or error");
	*made = false;
	voe_base_report_subject_push(path);
	glb = voe_platform_file_read(voe_platform_path_join(scratch, folder,
							    path),
				     scratch, &glb_size, error);
	ready = glb != NULL &&
		voe_game_models_levels_for(folder, path, glb, glb_size, remake,
					   scratch, workers, &size, made,
					   error) != NULL;
	voe_base_report_subject_pop();
	voe_base_arena_rewind(scratch, mark);
	return ready;
}

// The path of the world's `index`th file: the model rows' paths, then the
// emitters' textures.
static const char *path_at(const voe_ecs_world *world, uint32_t index)
{
	uint32_t rows = voe_3d_model_count(world);

	if (index < rows)
		return voe_3d_model_rows(world)[index].path;
	return voe_3d_emitter_rows(world)[index - rows].texture;
}

// Whether `path` is one an update reads: not empty and not in the store.
static bool unread(const voe_3d_models *models, const char *path)
{
	VOE_BASE_ASSERT(path != NULL, "asking after no path");
	return path[0] != '\0' && voe_3d_models_find(models, path) == NULL;
}

// Whether the world's `index`th of `paths` files is the first naming its
// path; a kind path past them always is. Each path is compared with every
// earlier one; a lookup set of the paths would lift that square.
static bool named_first(const voe_ecs_world *world, uint32_t paths,
			uint32_t index)
{
	bool earlier = false;

	VOE_BASE_ASSERT(world != NULL, "asking after no world");
	for (uint32_t j = 0; !earlier && index < paths && j < index; j++)
		earlier = strcmp(path_at(world, j), path_at(world, index)) == 0;
	return !earlier;
}

// How many distinct paths of `paths` files an update reads.
static unsigned count_unread(const voe_ecs_world *world,
			     const voe_3d_models *models, uint32_t paths)
{
	unsigned total = 0;

	VOE_BASE_ASSERT(models != NULL, "counting into no store");
	for (uint32_t i = 0; i < paths; i++)
		total += unread(models, path_at(world, i)) &&
			 named_first(world, paths, i);
	VOE_BASE_ASSERT(total <= paths, "more unread paths than paths");
	return total;
}

// The model rows' and emitters' files of the world, which path_at numbers.
static uint32_t world_paths(const voe_ecs_world *world)
{
	return voe_3d_model_count(world) + voe_3d_emitter_count(world);
}

// At most VOE_3D_MODELS: past that the store is full. Each is compared with
// every one kept and every world path; a lookup set would lift that square.
void voe_game_models_kinds_gather(const voe_ecs_world *world,
				  const voe_3d_models *models,
				  voe_game_models_kinds *kinds)
{
	uint32_t paths = world_paths(world);

	VOE_BASE_ASSERT(models != NULL && kinds != NULL, "no store or kinds");
	kinds->count = 0;
	for (uint32_t i = 0; i < voe_3d_models_count(models); i++) {
		const voe_assets_landscape *land =
			voe_3d_models_at(models, i)->landscape;

		for (uint32_t k = 0; land != NULL && k < land->kind_count; k++) {
			const char *path = land->kinds[k];
			bool known = !unread(models, path) ||
				     kinds->count == VOE_3D_MODELS;

			for (uint32_t j = 0; !known && j < kinds->count; j++)
				known = strcmp(kinds->paths[j], path) == 0;
			for (uint32_t j = 0; !known && j < paths; j++)
				known = strcmp(path_at(world, j), path) == 0;
			if (!known)
				kinds->paths[kinds->count++] = path;
		}
	}
	VOE_BASE_ASSERT(kinds->count <= VOE_3D_MODELS, "too many kinds kept");
}

// One loader call's running state: where it puts, what it reports into, and
// what it has counted and timed.
struct models_call {
	voe_3d_models *models;
	voe_render_device *device;
	voe_game_progress *progress;
	unsigned done;
	unsigned total;
	voe_base_error error;
	voe_game_models_failures failures;
	voe_game_models_times times;
};

// `batch` prepared over the store's pool and each file put in order,
// "Loading models" set before each. False when the store was full, the files
// after not put.
static bool batch_load(struct models_call *call, voe_game_models_batch *batch)
{
	bool room = true;

	VOE_BASE_ASSERT(call != NULL && batch != NULL, "no call or batch");
	voe_game_models_batch_prepare(
		batch, voe_3d_models_workers(call->models), &call->times);
	for (uint32_t k = 0; room && k < voe_game_models_batch_count(batch);
	     k++) {
		voe_game_progress_set(call->progress, "Loading models",
				      call->done, call->total);
		room = voe_game_models_batch_put(batch, call->models,
						 call->device, k,
						 &call->failures, &call->error,
						 &call->times);
		call->done += room;
	}
	VOE_BASE_ASSERT(call->total == 0 || call->done <= call->total,
			"more files put than counted");
	return room;
}

uint32_t voe_game_models_update_gather(const voe_ecs_world *world,
				       const voe_3d_models *models,
				       const voe_game_models_kinds *kinds,
				       uint32_t next, uint32_t room,
				       voe_game_models_batch *batch)
{
	uint32_t paths = world_paths(world);

	VOE_BASE_ASSERT(kinds != NULL && batch != NULL, "no kinds or batch");
	for (; next < paths + kinds->count &&
	       voe_game_models_batch_count(batch) < room;
	     next++) {
		const char *path = next < paths ? path_at(world, next) :
						  kinds->paths[next - paths];

		if (unread(models, path) && named_first(world, paths, next))
			voe_game_models_batch_add(batch, path, 0, false);
	}
	VOE_BASE_ASSERT(next <= paths + kinds->count, "gathered past the paths");
	return next;
}

void voe_game_models_dot_and_water(const voe_ecs_world *world,
				   voe_3d_models *models,
				   voe_render_device *device,
				   voe_game_models_failures *failures)
{
	voe_base_error error = VOE_BASE_OK;

	VOE_BASE_ASSERT(models != NULL && device != NULL && failures != NULL,
			"no store, device or failures");
	if (voe_3d_emitter_count(world) > 0 &&
	    voe_3d_models_find(models, "") == NULL &&
	    !voe_3d_models_load_dot(models, device, &error))
		voe_game_models_failure_count(failures, models, "");
	if (voe_3d_water_count(world) > 0 &&
	    voe_3d_models_water(models) == NULL &&
	    !voe_3d_models_load_water(models, device, &error))
		voe_game_models_failure_count(failures, models, "");
}

voe_game_models_failures voe_game_models_update(const voe_ecs_world *world,
						voe_3d_models *models,
						voe_render_device *device,
						const char *folder,
						voe_base_arena *scratch,
						voe_game_progress *progress)
{
	uint32_t paths = world_paths(world);
	uint32_t room = voe_base_workers_threads(voe_3d_models_workers(models));
	struct models_call call = { .models = models,
				    .device = device,
				    .progress = progress };
	bool going = true;
	voe_game_models_kinds kinds;

	VOE_BASE_ASSERT(models != NULL && device != NULL, "no store or device");
	VOE_BASE_ASSERT(folder != NULL && scratch != NULL,
			"no folder or scratch");
	voe_game_models_kinds_gather(world, models, &kinds);
	if (progress != NULL)
		call.total = count_unread(world, models, paths) + kinds.count;
	for (uint32_t next = 0; going && next < paths + kinds.count;) {
		voe_game_models_batch *batch =
			voe_game_models_batch_new(folder, room);
		bool any;

		next = voe_game_models_update_gather(world, models, &kinds, next,
						     room, batch);
		any = voe_game_models_batch_count(batch) > 0;
		if (any && voe_game_progress_stopped(progress)) {
			voe_game_models_batch_destroy(batch);
			voe_game_models_times_write(&call.times, "models",
					    VOE_GAME_MODELS_LINE);
			return call.failures;
		}
		// A full store said so once; the update ends.
		going = !any || (voe_3d_models_count(models) < VOE_3D_MODELS &&
				 batch_load(&call, batch));
		voe_game_models_batch_destroy(batch);
	}
	voe_game_progress_set(progress, "Loading models", call.done, call.total);
	voe_game_models_times_write(&call.times, "models",
					    VOE_GAME_MODELS_LINE);
	voe_game_models_dot_and_water(world, models, device, &call.failures);
	return call.failures;
}

bool voe_game_models_reload(voe_3d_models *models, voe_render_device *device,
			    const char *folder, const char *path,
			    voe_base_arena *scratch, voe_base_error *error)
{
	struct models_call call = { .models = models, .device = device };
	const voe_3d_model_entry *entry;
	voe_game_models_batch *batch;

	VOE_BASE_ASSERT(models != NULL && device != NULL, "no store or device");
	VOE_BASE_ASSERT(folder != NULL && path != NULL && scratch != NULL &&
				error != NULL,
			"no folder, path, scratch or error");
	*error = VOE_BASE_OK;
	entry = voe_3d_models_find(models, path);
	if (entry == NULL)
		return true;
	batch = voe_game_models_batch_new(folder, 1);
	voe_game_models_batch_add(batch, entry->path, entry->stamp, true);
	if (!batch_load(&call, batch))
		VOE_BASE_ASSERT(false, "a held path refused for room");
	voe_game_models_batch_destroy(batch);
	voe_game_models_times_write(&call.times, "models",
					    VOE_GAME_MODELS_LINE);
	*error = call.error;
	return call.failures.count == 0;
}

// `cooked`'s layer paths into `landscape`, and its paint, when cooked, as
// both planes in scratch: the second all nought when only one was cooked.
static void copy_layers(voe_assets_landscape *landscape,
			const voe_game_landscape *cooked, uint32_t heights,
			voe_base_arena *scratch)
{
	size_t plane = (size_t)heights * 4;

	VOE_BASE_ASSERT(cooked->layer_count <= VOE_ASSETS_LANDSCAPE_LAYERS,
			"a cooked landscape with too many layers");
	for (uint32_t k = 0; k < cooked->layer_count; k++) {
		size_t length = strlen(cooked->layers[k]);

		VOE_BASE_ASSERT(length < VOE_ASSETS_MATERIAL_PATH,
				"a cooked layer path too long");
		memcpy(landscape->layers[k], cooked->layers[k], length + 1);
	}
	landscape->layer_count = cooked->layer_count;
	if (cooked->paint == NULL)
		return;
	landscape->paint = voe_base_arena_push(scratch, plane * 2);
	memcpy(landscape->paint, cooked->paint,
	       cooked->layer_count > 4 ? plane * 2 : plane);
	if (cooked->layer_count <= 4)
		memset(landscape->paint + plane, 0, plane);
}

// `cooked`'s kind paths into `landscape`, and its copies handed on as they
// are: the store copies them into its own room and never writes these.
static void copy_foliage(voe_assets_landscape *landscape,
			 const voe_game_landscape *cooked)
{
	VOE_BASE_ASSERT(cooked->kind_count <= VOE_ASSETS_LANDSCAPE_KINDS,
			"a cooked landscape with too many kinds");
	VOE_BASE_ASSERT(cooked->copies != NULL || cooked->copy_count == 0,
			"cooked copies with no array");
	for (uint32_t k = 0; k < cooked->kind_count; k++) {
		size_t length = strlen(cooked->kinds[k]);

		VOE_BASE_ASSERT(length < VOE_ASSETS_LANDSCAPE_KIND_PATH,
				"a cooked kind path too long");
		memcpy(landscape->kinds[k], cooked->kinds[k], length + 1);
	}
	landscape->kind_count = cooked->kind_count;
	landscape->copies = (voe_assets_landscape_copy *)cooked->copies;
	landscape->copy_count = cooked->copy_count;
}

voe_game_models_failures
voe_game_models_landscapes(voe_3d_models *models, voe_render_device *device,
			   const voe_game_landscapes *landscapes,
			   voe_base_arena *scratch)
{
	voe_game_models_failures failures = { 0 };

	VOE_BASE_ASSERT(models != NULL && device != NULL, "no store or device");
	VOE_BASE_ASSERT(landscapes != NULL && scratch != NULL,
			"no landscapes or scratch");
	for (uint32_t i = 0; i < landscapes->count; i++) {
		const voe_game_landscape *cooked = &landscapes->landscapes[i];
		struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
		uint32_t heights = (cooked->cells + 1) * (cooked->cells + 1);
		voe_assets_landscape landscape = {
			.size = cooked->size,
			.cells = cooked->cells,
			.heights = voe_base_arena_push(scratch,
						       heights * sizeof(float)),
		};
		voe_base_error error = VOE_BASE_OK;

		// As assets/landscape.c reads them, so Play meets the editor's
		// heights to the bit.
		for (uint32_t h = 0; h < heights; h++)
			landscape.heights[h] =
				(float)((double)cooked->millimetres[h] / 1000.0);
		copy_layers(&landscape, cooked, heights, scratch);
		copy_foliage(&landscape, cooked);
		voe_base_report_subject_push(cooked->path);
		if (!voe_3d_models_load_landscape(models, device, cooked->path,
						  0, &landscape, &error))
			voe_game_models_failure_count(&failures, models,
						      cooked->path);
		voe_base_report_subject_pop();
		voe_base_arena_rewind(scratch, mark);
	}
	return failures;
}

uint32_t voe_game_models_watch_gather(const voe_3d_models *models,
				      uint32_t next, uint32_t room,
				      voe_game_models_batch *batch)
{
	VOE_BASE_ASSERT(models != NULL && batch != NULL, "no store or batch");
	for (; next < voe_3d_models_count(models) &&
	       voe_game_models_batch_count(batch) < room;
	     next++) {
		const voe_3d_model_entry *entry = voe_3d_models_at(models, next);

		// A landscape is never re-read by stamp (0379 point 2), nor a
		// material, which the editor reloads itself (0399 point 7).
		if (entry->landscape == NULL && !entry->material)
			voe_game_models_batch_add(batch, entry->path,
						  entry->stamp, false);
	}
	VOE_BASE_ASSERT(voe_game_models_batch_count(batch) <= room,
			"gathered past the room");
	return next;
}

voe_game_models_failures voe_game_models_watch(voe_3d_models *models,
					       voe_render_device *device,
					       const char *folder,
					       voe_base_arena *scratch)
{
	uint32_t room = voe_base_workers_threads(voe_3d_models_workers(models));
	struct models_call call = { .models = models, .device = device };

	VOE_BASE_ASSERT(models != NULL && device != NULL, "no store or device");
	VOE_BASE_ASSERT(folder != NULL && scratch != NULL,
			"no folder or scratch");
	for (uint32_t i = 0; i < voe_3d_models_count(models);) {
		voe_game_models_batch *batch =
			voe_game_models_batch_new(folder, room);

		i = voe_game_models_watch_gather(models, i, room, batch);
		// Every path is held, so the store has room for each.
		if (!batch_load(&call, batch))
			VOE_BASE_ASSERT(false, "a held path refused for room");
		voe_game_models_batch_destroy(batch);
	}
	voe_game_models_times_write(&call.times, "models",
					    VOE_GAME_MODELS_LINE);
	return call.failures;
}
