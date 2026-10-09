// The editor's one model store models.h describes: made empty, emptied on a
// different folder, filled and re-read through game/models.h once a frame,
// cleared through the device and destroyed, and handed out read-only; its
// landscapes' heights written in the frame, written on Save and read again,
// and one reshaped, written at once and loaded again at the next update.
#include "models.h"

#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>

#include <game/models.h>

#include <platform/file.h>
#include <platform/path.h>

#include <stdlib.h>
#include <string.h>

// How often the store's files are asked for their stamps, in seconds.
#define WATCH_SECONDS 1.0

struct voe_editor_models {
	voe_3d_models *store;
	// The folder the store was last read against, the store's own copy;
	// NULL before any and while the project is untitled.
	char *folder;
	// The frame clock's reading at the last watch.
	double looked;
	// A landscape whose size was written, to load again at the next
	// update, the store's own copy; NULL for none.
	char *resized;
};

static void reread(voe_3d_models *store, voe_render_device *device,
		   const char *folder, const char *path,
		   voe_base_arena *scratch);

voe_editor_models *voe_editor_models_new(void)
{
	voe_editor_models *models = calloc(1, sizeof(*models));

	VOE_BASE_ASSERT(models != NULL, "out of memory making the model store");
	models->store = voe_3d_models_new();
	VOE_BASE_ASSERT(models->store != NULL, "made a model store with none");
	return models;
}

void voe_editor_models_destroy(voe_editor_models *models,
			       voe_render_device *device)
{
	if (models == NULL)
		return;
	VOE_BASE_ASSERT(device != NULL, "clearing models with no device");
	voe_3d_models_clear(models->store, device);
	voe_3d_models_destroy(models->store);
	free(models->folder);
	free(models->resized);
	free(models);
}

// Empties the store and keeps a copy of `folder` (NULL for untitled) when it
// is not the one last read against.
static void follow_folder(voe_editor_models *models, const char *folder,
			  voe_render_device *device)
{
	if (folder == NULL && models->folder == NULL)
		return;
	if (folder != NULL && models->folder != NULL &&
	    strcmp(folder, models->folder) == 0)
		return;
	voe_3d_models_clear(models->store, device);
	free(models->folder);
	models->folder = NULL;
	free(models->resized);
	models->resized = NULL;
	if (folder != NULL) {
		size_t size = strlen(folder) + 1;

		models->folder = malloc(size);
		VOE_BASE_ASSERT(models->folder != NULL,
				"out of memory copying the models' folder");
		memcpy(models->folder, folder, size);
	}
	VOE_BASE_ASSERT(voe_3d_models_count(models->store) == 0,
			"a cleared store still holds models");
}

// The notice for a call that failed: the first path and the reason, the last
// phrase of the first error kept since the caller's clear.
static void say_failure(voe_editor_notice *notice,
			voe_game_models_failures failures)
{
	const char *kept = voe_base_report_error_first();
	const char *category = kept != NULL ? strrchr(kept, ':') : NULL;

	VOE_BASE_ASSERT(notice != NULL, "saying a failure into no notice");
	if (failures.count == 0)
		return;
	if (category != NULL)
		category += category[1] == ' ' ? 2 : 1;
	else
		category = kept != NULL ? kept : "could not be read";
	voe_editor_notice_set(notice, "Could not read %s: %s",
			      failures.first != NULL ? failures.first : "a model",
			      category);
}

void voe_editor_models_update(voe_editor_models *models,
			      voe_editor_session *session,
			      voe_render_device *device,
			      voe_base_arena *scratch, double now,
			      voe_game_progress *progress)
{
	VOE_BASE_ASSERT(models != NULL && session != NULL,
			"updating no store or no session");
	VOE_BASE_ASSERT(session->project != NULL && device != NULL &&
				scratch != NULL,
			"updating models with no project, device or scratch");
	follow_folder(models, session->project->folder, device);
	if (models->folder == NULL)
		return;
	if (models->resized != NULL) {
		struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);

		reread(models->store, device, models->folder, models->resized,
		       scratch);
		voe_base_arena_rewind(scratch, mark);
		free(models->resized);
		models->resized = NULL;
	}
	voe_base_report_error_clear();
	say_failure(&session->notice,
		    voe_game_models_update(session->project->world,
					   models->store, device,
					   models->folder, scratch, progress));
	if (now - models->looked < WATCH_SECONDS)
		return;
	models->looked = now;
	voe_base_report_error_clear();
	say_failure(&session->notice,
		    voe_game_models_watch(models->store, device, models->folder,
					  scratch));
}

void voe_editor_models_frame(voe_editor_models *models,
			     voe_render_device *device)
{
	VOE_BASE_ASSERT(models != NULL && models->store != NULL,
			"writing landscapes of no store");
	VOE_BASE_ASSERT(device != NULL, "writing landscapes with no device");
	voe_3d_models_landscape_frame(models->store, device);
}

// Whether `entry` is a landscape changed since it was loaded or saved.
static bool edited_landscape(const voe_3d_model_entry *entry)
{
	VOE_BASE_ASSERT(entry != NULL, "asking after no entry");
	return entry->landscape != NULL && entry->edited;
}

bool voe_editor_models_save(voe_editor_models *models, const char *folder,
			    voe_base_arena *scratch, voe_editor_notice *why)
{
	uint32_t count;

	VOE_BASE_ASSERT(models != NULL && folder != NULL,
			"saving landscapes of no store or into no folder");
	VOE_BASE_ASSERT(scratch != NULL && why != NULL,
			"saving landscapes with no scratch or notice");
	count = voe_3d_models_count(models->store);
	for (uint32_t i = 0; i < count; i++) {
		const voe_3d_model_entry *entry =
			voe_3d_models_at(models->store, i);
		struct voe_base_arena_mark mark;
		voe_assets_landscape_text text;
		voe_base_error error = VOE_BASE_OK;
		bool written;

		if (!edited_landscape(entry))
			continue;
		mark = voe_base_arena_mark(scratch);
		text = voe_assets_landscape_write(entry->landscape, scratch);
		written = voe_platform_file_write(
			voe_platform_path_join(scratch, folder, entry->path),
			(const uint8_t *)text.text, text.size, &error);
		voe_base_arena_rewind(scratch, mark);
		if (!written) {
			voe_editor_notice_set(why, "Could not save %s: %s",
					      entry->path,
					      voe_base_error_string(error));
			return false;
		}
		voe_3d_models_landscape_saved(models->store, entry->path);
	}
	return true;
}

// Reads `path` under `folder` and loads it over its entry; a file that will
// not read is kept failed as game/models.c keeps one. `path` is a copy, since
// a load replaces the entry that held it.
static void reread(voe_3d_models *store, voe_render_device *device,
		   const char *folder, const char *path,
		   voe_base_arena *scratch)
{
	const char *full = voe_platform_path_join(scratch, folder, path);
	voe_base_error error = VOE_BASE_OK;
	uint64_t stamp = 0;
	size_t size = 0;
	const uint8_t *bytes;

	VOE_BASE_ASSERT(path[0] != '\0', "re-reading a landscape with no path");
	bytes = voe_platform_file_stamp(full, &stamp) ?
			voe_platform_file_read(full, scratch, &size, &error) :
			NULL;
	if (bytes == NULL) {
		voe_3d_models_fail(store, path, 0);
		VOE_BASE_ERROR("editor", "could not read %s again", path);
		return;
	}
	if (!voe_3d_models_load(store, device, path, stamp, bytes, size,
				&error))
		VOE_BASE_ERROR("editor", "could not read %s again: %s", path,
			       voe_base_error_string(error));
}

void voe_editor_models_revert(voe_editor_models *models, const char *folder,
			      voe_render_device *device,
			      voe_base_arena *scratch)
{
	uint32_t count;

	VOE_BASE_ASSERT(models != NULL && models->store != NULL,
			"reverting landscapes of no store");
	VOE_BASE_ASSERT(device != NULL && scratch != NULL,
			"reverting landscapes with no device or scratch");
	if (folder == NULL)
		return;
	count = voe_3d_models_count(models->store);
	for (uint32_t i = 0; i < count; i++) {
		const voe_3d_model_entry *entry =
			voe_3d_models_at(models->store, i);
		struct voe_base_arena_mark mark;
		size_t length;
		char *path;

		if (!edited_landscape(entry))
			continue;
		mark = voe_base_arena_mark(scratch);
		length = strlen(entry->path) + 1;
		path = voe_base_arena_push(scratch, length);
		VOE_BASE_ASSERT(path != NULL, "no room to copy a landscape's path");
		memcpy(path, entry->path, length);
		reread(models->store, device, folder, path, scratch);
		voe_base_arena_rewind(scratch, mark);
	}
}

voe_3d_landscape_rect voe_editor_models_brush(voe_editor_models *models,
					      const char *path,
					      const voe_3d_brush *brush,
					      float x, float z, float seconds,
					      voe_base_arena *scratch)
{
	VOE_BASE_ASSERT(models != NULL && models->store != NULL,
			"stamping a brush into no store");
	VOE_BASE_ASSERT(path != NULL && brush != NULL && scratch != NULL,
			"stamping no brush, onto no path or with no scratch");
	return voe_3d_models_landscape_brush(models->store, path, brush, x, z,
					     seconds, scratch);
}

void voe_editor_models_put(voe_editor_models *models, const char *path,
			   voe_3d_landscape_rect rect, const float *values)
{
	VOE_BASE_ASSERT(models != NULL && models->store != NULL,
			"putting heights into no store");
	VOE_BASE_ASSERT(path != NULL && values != NULL,
			"putting no heights or onto no path");
	voe_3d_models_landscape_put(models->store, path, rect, values);
}

void voe_editor_models_rename(voe_editor_models *models, const char *from,
			      const char *to)
{
	VOE_BASE_ASSERT(models != NULL && models->store != NULL,
			"renaming in no store");
	VOE_BASE_ASSERT(from != NULL && to != NULL, "renaming no path");
	voe_3d_models_rename(models->store, from, to);
}

// `path`'s grid into `out`: the loaded entry's own, else read from its file
// into `scratch`. False with `why` naming the file when it will not read.
static bool landscape_got(const voe_editor_models *models, const char *folder,
			  const char *path, voe_base_arena *scratch,
			  voe_assets_landscape *out, voe_editor_notice *why)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(models->store, path);
	voe_base_error error = VOE_BASE_OK;
	const uint8_t *bytes;
	size_t count = 0;

	VOE_BASE_ASSERT(folder != NULL && path != NULL && out != NULL,
			"a landscape from no folder or path, or into nowhere");
	if (entry != NULL && entry->landscape != NULL) {
		*out = *entry->landscape;
		return true;
	}
	bytes = voe_platform_file_read(voe_platform_path_join(scratch, folder,
							      path),
				       scratch, &count, &error);
	if (bytes == NULL ||
	    !voe_assets_landscape_read((const char *)bytes, count, scratch, out,
				       &error)) {
		voe_editor_notice_set(why, "Could not read %s: %s", path,
				      voe_base_error_string(error));
		return false;
	}
	return true;
}

bool voe_editor_models_landscape_found(voe_editor_models *models,
				       const char *folder, const char *path,
				       voe_base_arena *scratch, float *size,
				       uint32_t *cells, voe_editor_notice *why)
{
	struct voe_base_arena_mark mark;
	voe_assets_landscape grid;
	bool found;

	VOE_BASE_ASSERT(models != NULL && scratch != NULL && size != NULL &&
				cells != NULL,
			"a landscape's shape from no store, scratch or out");
	VOE_BASE_ASSERT(why != NULL, "a landscape's size with no notice");
	mark = voe_base_arena_mark(scratch);
	found = landscape_got(models, folder, path, scratch, &grid, why);
	if (found) {
		*size = grid.size;
		*cells = grid.cells;
	}
	voe_base_arena_rewind(scratch, mark);
	return found;
}

bool voe_editor_models_landscape_shape(voe_editor_models *models,
				       const char *folder, const char *path,
				       float size, uint32_t cells,
				       voe_base_arena *scratch,
				       voe_editor_notice *why)
{
	const voe_3d_model_entry *entry;
	struct voe_base_arena_mark mark;
	voe_assets_landscape grid;
	voe_assets_landscape_text text;
	voe_base_error error = VOE_BASE_OK;
	bool written = false;

	VOE_BASE_ASSERT(models != NULL && scratch != NULL && why != NULL,
			"sizing a landscape with no store, scratch or notice");
	VOE_BASE_ASSERT(size >= VOE_ASSETS_LANDSCAPE_SIZE_MIN &&
				size <= VOE_ASSETS_LANDSCAPE_SIZE_MAX,
			"a landscape's size out of range");
	VOE_BASE_ASSERT(cells >= 4 && cells <= VOE_ASSETS_LANDSCAPE_CELLS_MAX &&
				cells % 4 == 0,
			"a landscape's cells out of range");
	mark = voe_base_arena_mark(scratch);
	if (!landscape_got(models, folder, path, scratch, &grid, why))
		goto rewind;
	if (grid.cells != cells)
		grid = voe_assets_landscape_resample(&grid, cells, scratch);
	grid.size = size;
	text = voe_assets_landscape_write(&grid, scratch);
	written = voe_platform_file_write(
		voe_platform_path_join(scratch, folder, path),
		(const uint8_t *)text.text, text.size, &error);
	if (!written) {
		voe_editor_notice_set(why, "Could not save %s: %s", path,
				      voe_base_error_string(error));
		goto rewind;
	}
	entry = voe_3d_models_find(models->store, path);
	if (entry != NULL && entry->landscape != NULL) {
		size_t length = strlen(path) + 1;

		voe_3d_models_landscape_saved(models->store, path);
		free(models->resized);
		models->resized = malloc(length);
		VOE_BASE_ASSERT(models->resized != NULL,
				"out of memory keeping a resized landscape");
		memcpy(models->resized, path, length);
	}
rewind:
	voe_base_arena_rewind(scratch, mark);
	return written;
}

const voe_3d_models *voe_editor_models_store(const voe_editor_models *models)
{
	VOE_BASE_ASSERT(models != NULL, "reading no model store");
	VOE_BASE_ASSERT(models->store != NULL, "a model store with none");
	return models->store;
}
