// The editor's one model store models.h describes: made empty, emptied on a
// different folder, its materials' and parts' tables read again then, filled,
// watched and re-read through game/models.h once a frame, one prepared model
// loaded again on asking, the tables'
// materials loaded and parts given after, cleared through the device and
// destroyed, and handed out read-only; its landscapes' heights and paint
// stamped, put back and their layers set, their foliage scattered, erased,
// put back and its kinds set, heights and materials' records
// written in the frame, landscapes written on Save and read again, and one
// reshaped, written at once and loaded again at the next update; an edited
// material set live, or loaded again at the next update when a map changed,
// as is every held material a read of the tables found new or changed. A read
// that fails is said in the session's notice, the kept line whole; a landscape
// re-read names its path as the subject.
//
// The store carries the program's loading pool (0448 point 2): one thread
// fewer than the machine's cores, made with the store and destroyed after it.
// Between frames the update goes through game/models_loader.h (0448 point 5),
// the watch too; a splash wait's call, with its progress, loads whole.
#include "models.h"

#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>
#include <base/workers.h>

#include <game/models.h>
#include <game/models_loader.h>

#include <platform/cores.h>
#include <platform/file.h>
#include <platform/path.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// How often the store's files are asked for their stamps, in seconds.
#define WATCH_SECONDS 1.0

struct voe_editor_models {
	voe_3d_models *store;
	// The loading pool set on the store, outliving it.
	voe_base_workers *workers;
	// Reads and bakes the update's and the watch's files beside the frames,
	// over `workers`; destroyed before them.
	voe_game_models_loader *loader;
	// The project's materials and parts, read on a different folder.
	voe_editor_materials *materials;
	voe_editor_parts *parts;
	// The materials as the read before the last, to find what it changed;
	// swapped with `materials` at each read, since a table is never copied.
	voe_editor_materials *before;
	// The folder the store was last read against, the store's own copy;
	// NULL before any and while the project is untitled.
	char *folder;
	// The frame clock's reading at the last watch.
	double looked;
	// A landscape whose size was written, to load again at the next
	// update, the store's own copy; NULL for none.
	char *resized;
	// The materials to load again at the next update, `reloads` of them.
	char reload[VOE_EDITOR_MATERIALS][VOE_ASSETS_MATERIAL_PATH];
	uint32_t reloads;
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
	models->workers = voe_base_workers_new(voe_platform_cores() - 1);
	voe_3d_models_workers_set(models->store, models->workers);
	models->loader = voe_game_models_loader_new();
	VOE_BASE_ASSERT(models->loader != NULL, "made a models loader with none");
	models->materials = calloc(1, sizeof(*models->materials));
	models->before = calloc(1, sizeof(*models->before));
	models->parts = calloc(1, sizeof(*models->parts));
	VOE_BASE_ASSERT(models->materials != NULL && models->before != NULL &&
				models->parts != NULL,
			"out of memory making the materials' and parts' tables");
	return models;
}

void voe_editor_models_destroy(voe_editor_models *models,
			       voe_render_device *device)
{
	if (models == NULL)
		return;
	VOE_BASE_ASSERT(device != NULL, "clearing models with no device");
	voe_game_models_loader_destroy(models->loader);
	voe_3d_models_clear(models->store, device);
	voe_3d_models_destroy(models->store);
	voe_base_workers_destroy(models->workers);
	free(models->materials);
	free(models->before);
	free(models->parts);
	free(models->folder);
	free(models->resized);
	free(models);
}

// Empties the store, keeps a copy of `folder` (NULL for untitled) and reads
// its materials and parts when it is not the one last read against.
static void follow_folder(voe_editor_models *models, const char *folder,
			  voe_render_device *device, voe_base_arena *scratch)
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
	models->reloads = 0;
	if (folder != NULL) {
		size_t size = strlen(folder) + 1;

		models->folder = malloc(size);
		VOE_BASE_ASSERT(models->folder != NULL,
				"out of memory copying the models' folder");
		memcpy(models->folder, folder, size);
	}
	voe_editor_materials_read(models->materials, models->folder, scratch);
	voe_editor_parts_read(models->parts, models->folder, scratch);
	VOE_BASE_ASSERT(voe_3d_models_count(models->store) == 0,
			"a cleared store still holds models");
}

// The notice for a call that failed: the first error kept since the caller's
// clear, whole, named by the first path when it does not name it (0445 point 6).
static void say_failure(voe_editor_notice *notice,
			voe_game_models_failures failures)
{
	VOE_BASE_ASSERT(notice != NULL, "saying a failure into no notice");
	if (failures.count == 0)
		return;
	voe_editor_notice_from_report(notice, failures.first != NULL ?
						      failures.first :
						      "a model");
}

// `path` waits to be loaded again at the next update, once however often it
// is asked; a full wait leaves it out with a line on stderr.
static void reload_add(voe_editor_models *models, const char *path)
{
	VOE_BASE_ASSERT(models != NULL && path != NULL, "waiting on no material");
	VOE_BASE_ASSERT(models->reloads <= VOE_EDITOR_MATERIALS,
			"more reloads waiting than their room");
	for (uint32_t i = 0; i < models->reloads; i++)
		if (strcmp(models->reload[i], path) == 0)
			return;
	if (models->reloads == VOE_EDITOR_MATERIALS) {
		VOE_BASE_ERROR("editor", "left %s as it was: %u reloads wait",
			       path, VOE_EDITOR_MATERIALS);
		return;
	}
	snprintf(models->reload[models->reloads], VOE_ASSETS_MATERIAL_PATH,
		 "%s", path);
	models->reloads++;
}

// The materials waiting in `reload` loaded again from their table rows, at
// stamp 0 as the update loads one; a row gone from the table is let go, a
// failure said in `notice`.
static void material_reload(voe_editor_models *models,
			    voe_render_device *device, voe_base_arena *scratch,
			    voe_editor_notice *notice)
{
	VOE_BASE_ASSERT(models != NULL && models->folder != NULL,
			"reloading a material of no project");
	VOE_BASE_ASSERT(device != NULL && scratch != NULL && notice != NULL,
			"reloading a material with no device, scratch or notice");
	for (uint32_t i = 0; i < models->reloads; i++) {
		const voe_game_material *row = voe_editor_materials_find(
			models->materials, models->reload[i]);
		voe_base_error error = VOE_BASE_OK;

		if (row == NULL)
			continue;
		voe_base_report_error_clear();
		if (!voe_game_models_material_load(models->store, device,
						   models->folder, row, 0,
						   scratch, &error))
			voe_editor_notice_set(notice, "Could not read %s: %s",
					      row->path,
					      voe_base_error_string(error));
	}
	models->reloads = 0;
}

void voe_editor_models_update(voe_editor_models *models,
			      voe_editor_session *session,
			      voe_render_device *device,
			      voe_base_arena *scratch, double now,
			      voe_game_progress *progress)
{
	voe_game_materials table;
	voe_game_parts parts;
	bool watch;

	VOE_BASE_ASSERT(models != NULL && session != NULL,
			"updating no store or no session");
	VOE_BASE_ASSERT(session->project != NULL && device != NULL &&
				scratch != NULL,
			"updating models with no project, device or scratch");
	follow_folder(models, session->project->folder, device, scratch);
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
	// The watch before the materials, so a model it reloads is given its
	// parts before it is drawn.
	watch = now - models->looked >= WATCH_SECONDS;
	if (watch)
		models->looked = now;
	if (progress == NULL) {
		voe_base_report_error_clear();
		say_failure(&session->notice,
			    voe_game_models_loader_step(
				    models->loader, session->project->world,
				    models->store, device, models->folder,
				    watch, scratch));
	} else {
		voe_base_report_error_clear();
		say_failure(&session->notice,
			    voe_game_models_update(session->project->world,
						   models->store, device,
						   models->folder, scratch,
						   progress));
		if (watch) {
			voe_base_report_error_clear();
			say_failure(&session->notice,
				    voe_game_models_watch(models->store, device,
							  models->folder,
							  scratch));
		}
	}
	table = voe_editor_materials_table(models->materials);
	parts = voe_editor_parts_table(models->parts);
	voe_base_report_error_clear();
	say_failure(&session->notice,
		    voe_game_models_materials(session->project->world,
					      models->store, device,
					      models->folder, &table, &parts,
					      scratch));
	material_reload(models, device, scratch, &session->notice);
}

// Between frames, since a load waits for the card to go idle. A path the
// store lacks is left alone: it loads with the new levels when first placed
// (0431). The materials call runs on every update, so no reload need be
// remembered for the parts.
void voe_editor_models_reload(voe_editor_models *models,
			      voe_editor_session *session,
			      voe_render_device *device, const char *path,
			      voe_base_arena *scratch)
{
	const char *folder;
	struct voe_base_arena_mark mark;
	voe_base_error error = VOE_BASE_OK;

	VOE_BASE_ASSERT(models != NULL && session != NULL &&
				session->project != NULL,
			"reloading a model of no store or no project");
	VOE_BASE_ASSERT(device != NULL && path != NULL && scratch != NULL,
			"reloading a model with no device, path or scratch");
	folder = session->project->folder;
	if (folder == NULL || models->folder == NULL ||
	    strcmp(folder, models->folder) != 0)
		return;
	mark = voe_base_arena_mark(scratch);
	voe_base_report_error_clear();
	if (!voe_game_models_reload(models->store, device, models->folder, path,
				    scratch, &error))
		say_failure(&session->notice,
			    (voe_game_models_failures){ .count = 1,
							.first = path });
	voe_base_arena_rewind(scratch, mark);
}

void voe_editor_models_frame(voe_editor_models *models,
			     voe_render_device *device)
{
	VOE_BASE_ASSERT(models != NULL && models->store != NULL,
			"writing landscapes of no store");
	VOE_BASE_ASSERT(device != NULL, "writing landscapes with no device");
	voe_3d_models_landscape_frame(models->store, device);
	voe_3d_models_material_frame(models->store, device);
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

// Reads `path` under `folder` and loads it over its entry, `path` the reports'
// subject; a file that will not read is kept failed as game/models.c keeps
// one. A refused load says nothing here: its cause has its line (0445 point
// 3). `path` is a copy, since a load replaces the entry that held it.
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
	voe_base_report_subject_push(path);
	bytes = voe_platform_file_stamp(full, &stamp) ?
			voe_platform_file_read(full, scratch, &size, &error) :
			NULL;
	if (bytes == NULL) {
		voe_3d_models_fail(store, path, 0);
		VOE_BASE_ERROR("editor", "could not be read again");
	} else if (!voe_3d_models_load(store, device, path, stamp, bytes, size,
				       &error)) {
		VOE_BASE_ASSERT(error != VOE_BASE_OK,
				"a refused re-read with no error");
	}
	voe_base_report_subject_pop();
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

void voe_editor_models_put_paint(voe_editor_models *models, const char *path,
				 voe_3d_landscape_rect rect,
				 const uint8_t *values)
{
	VOE_BASE_ASSERT(models != NULL && models->store != NULL,
			"putting paint into no store");
	VOE_BASE_ASSERT(path != NULL && values != NULL,
			"putting no paint or onto no path");
	voe_3d_models_landscape_put_paint(models->store, path, rect, values);
}

void voe_editor_models_layer_set(voe_editor_models *models, const char *path,
				 uint32_t index, const char *material)
{
	VOE_BASE_ASSERT(models != NULL && models->store != NULL,
			"setting a layer in no store");
	VOE_BASE_ASSERT(path != NULL && material != NULL,
			"setting no material or onto no path");
	voe_3d_models_landscape_layer_set(models->store, path, index, material);
}

uint32_t voe_editor_models_scatter(voe_editor_models *models, const char *path,
				   const voe_3d_foliage_brush *brush, float x,
				   float z, float seconds, uint64_t *seed,
				   voe_assets_landscape_copy *added)
{
	VOE_BASE_ASSERT(models != NULL && models->store != NULL,
			"scattering into no store");
	VOE_BASE_ASSERT(path != NULL && brush != NULL && seed != NULL &&
				added != NULL,
			"scattering no brush, onto no path, or with no seed or room");
	return voe_3d_models_landscape_scatter(models->store, path, brush, x, z,
					       seconds, seed, added);
}

uint32_t voe_editor_models_erase(voe_editor_models *models, const char *path,
				 const voe_3d_foliage_brush *brush, float x,
				 float z, voe_assets_landscape_copy *removed,
				 uint32_t room)
{
	VOE_BASE_ASSERT(models != NULL && models->store != NULL,
			"erasing from no store");
	VOE_BASE_ASSERT(path != NULL && brush != NULL && removed != NULL,
			"erasing with no brush, on no path or into no room");
	return voe_3d_models_landscape_erase(models->store, path, brush, x, z,
					     removed, room);
}

void voe_editor_models_put_copies(voe_editor_models *models, const char *path,
				  const voe_assets_landscape_copy *copies,
				  uint32_t count, bool add)
{
	VOE_BASE_ASSERT(models != NULL && models->store != NULL,
			"putting copies into no store");
	VOE_BASE_ASSERT(path != NULL && (count == 0 || copies != NULL),
			"putting no copies or onto no path");
	voe_3d_models_landscape_put_copies(models->store, path, copies, count,
					   add);
}

void voe_editor_models_kind_set(voe_editor_models *models, const char *path,
				uint32_t index, const char *model)
{
	VOE_BASE_ASSERT(models != NULL && models->store != NULL,
			"setting a kind in no store");
	VOE_BASE_ASSERT(path != NULL && model != NULL,
			"setting no model or onto no path");
	voe_3d_models_landscape_kind_set(models->store, path, index, model);
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

voe_editor_materials *voe_editor_models_materials(voe_editor_models *models)
{
	VOE_BASE_ASSERT(models != NULL, "the materials of no store");
	VOE_BASE_ASSERT(models->materials != NULL, "a store with no table");
	return models->materials;
}

void voe_editor_models_materials_read(voe_editor_models *models,
				      const char *folder,
				      voe_base_arena *scratch)
{
	voe_editor_materials *swap;

	VOE_BASE_ASSERT(models != NULL && models->materials != NULL,
			"reading materials into no store");
	VOE_BASE_ASSERT(scratch != NULL, "reading materials with no scratch");
	swap = models->before;
	models->before = models->materials;
	models->materials = swap;
	voe_editor_materials_read(models->materials, folder, scratch);
	voe_editor_parts_read(models->parts, folder, scratch);
	// A false difference, a padding byte, costs one reload and no more.
	for (uint32_t i = 0; i < models->materials->count; i++) {
		const voe_game_material *row = &models->materials->rows[i];
		const voe_game_material *old =
			voe_editor_materials_find(models->before, row->path);

		if ((old == NULL ||
		     memcmp(&old->values, &row->values, sizeof row->values) !=
			     0) &&
		    voe_3d_models_find(models->store, row->path) != NULL)
			reload_add(models, row->path);
	}
}

void voe_editor_models_material_changed(voe_editor_models *models,
					const char *path, bool maps)
{
	const voe_game_material *row;

	VOE_BASE_ASSERT(models != NULL && models->materials != NULL,
			"a material changed in no store");
	VOE_BASE_ASSERT(path != NULL, "no material changed");
	row = voe_editor_materials_find(models->materials, path);
	if (row == NULL)
		return;
	if (!maps) {
		voe_3d_models_material_set(models->store, path, &row->values);
		return;
	}
	reload_add(models, path);
}

const voe_3d_models *voe_editor_models_store(const voe_editor_models *models)
{
	VOE_BASE_ASSERT(models != NULL, "reading no model store");
	VOE_BASE_ASSERT(models->store != NULL, "a model store with none");
	return models->store;
}
