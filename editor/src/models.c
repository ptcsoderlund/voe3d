// The editor's one model store models.h describes, all but its landscape
// edits, which are models_edit.c's: made empty, emptied on a different folder,
// its materials' and parts' tables read again then, filled, watched and
// re-read through game/models.h once a frame, one prepared model loaded again
// on asking, the tables' materials loaded and parts given after, cleared
// through the device and destroyed, and handed out read-only; its landscapes'
// heights and materials' records written in the frame, landscapes written on
// Save and read again, and one reshaped loaded again at the next update; an
// edited material set live, or loaded again at the next update when a map
// changed, as is every held material a read of the tables found new or
// changed. A read that fails is said in the session's notice, the kept line
// whole; a landscape re-read names its path as the subject. A map with no
// fresh copy is queued and compressed on the GPU a step a frame outside a
// wait, inline under one, every row naming a written map loaded again at the
// next update. Once a second, after the models' watch, the maps the materials
// name are stamped, and the rows naming a changed one load again, the queue
// compressing it (0477 point 5). Between frames the update and the watch load
// whole (0459 point 3). A landscape taken in from disk waits as a copy in an
// arena of its own, at most LANDSCAPE_TAKES (models_store.h) at once, and
// loads at the next update; a bigger count lifts it. The species are read
// with the materials and grown after the watch: whole under a splash, else on
// the one job a species at a time, a variation put an update. A different
// folder or a whole grow lets a busy job go by destroying it, waiting out its
// burst, and making another: the job has no cancel; one would lift that. The
// struct is models_store.h's, shared with models_edit.c.
#include "models_store.h"

#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>

#include <game/models.h>

#include <platform/file.h>
#include <platform/path.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// How often the store's files are asked for their stamps, in seconds.
#define WATCH_SECONDS 1.0

static void reread(voe_3d_models *store, voe_render_device *device,
		   const char *folder, const char *path,
		   voe_base_arena *scratch);
static void take_load(voe_3d_models *store, voe_render_device *device,
		      const struct landscape_take *take);

voe_editor_models *voe_editor_models_new(void)
{
	voe_editor_models *models = calloc(1, sizeof(*models));

	VOE_BASE_ASSERT(models != NULL, "out of memory making the model store");
	models->store = voe_3d_models_new();
	VOE_BASE_ASSERT(models->store != NULL, "made a model store with none");
	models->compressing = voe_game_compressing_new();
	VOE_BASE_ASSERT(models->compressing != NULL,
			"made a compressing queue with none");
	models->materials = calloc(1, sizeof(*models->materials));
	models->before = calloc(1, sizeof(*models->before));
	models->parts = calloc(1, sizeof(*models->parts));
	models->trees = calloc(1, sizeof(*models->trees));
	models->trees_before = calloc(1, sizeof(*models->trees_before));
	VOE_BASE_ASSERT(models->materials != NULL && models->before != NULL &&
				models->parts != NULL && models->trees != NULL &&
				models->trees_before != NULL,
			"out of memory making the materials', parts' and species' "
			"tables");
	models->job = voe_game_trees_job_new();
	VOE_BASE_ASSERT(models->job != NULL, "made a trees job with none");
	return models;
}

// Every waiting take let go, its arena destroyed.
static void takes_clear(voe_editor_models *models)
{
	VOE_BASE_ASSERT(models->takes <= LANDSCAPE_TAKES,
			"more takes waiting than their room");
	for (uint32_t i = 0; i < models->takes; i++)
		voe_base_arena_destroy(models->take[i].arena);
	models->takes = 0;
	VOE_BASE_ASSERT(models->takes == 0, "takes left waiting");
}

void voe_editor_models_destroy(voe_editor_models *models,
			       voe_render_device *device)
{
	if (models == NULL)
		return;
	VOE_BASE_ASSERT(device != NULL, "clearing models without a device");
	voe_game_trees_job_destroy(models->job);
	takes_clear(models);
	voe_3d_models_clear(models->store, device);
	voe_3d_models_destroy(models->store);
	voe_game_compressing_destroy(models->compressing, device);
	free(models->materials);
	free(models->before);
	free(models->parts);
	free(models->trees);
	free(models->trees_before);
	free(models->folder);
	free(models->resized);
	free(models);
}

// The job let go, its burst waited out, and an idle one made in its place: a
// store emptied or grown whole has no use for the species it was growing, and
// the job has no call that drops one.
static void job_cancel(voe_editor_models *models)
{
	VOE_BASE_ASSERT(models != NULL && models->job != NULL,
			"cancelling no trees job");
	if (models->growing[0] == '\0')
		return;
	voe_game_trees_job_destroy(models->job);
	models->job = voe_game_trees_job_new();
	models->growing[0] = '\0';
	VOE_BASE_ASSERT(models->job != NULL, "made a trees job with none");
}

// The species read again from `folder`, each row marked that is new, changed,
// or was marked before the read. A false difference, a padding byte, costs one
// regrow and no more.
static void trees_read(voe_editor_models *models, const char *folder,
		       voe_base_arena *scratch)
{
	voe_editor_trees *swap = models->trees_before;
	voe_editor_trees *trees;

	VOE_BASE_ASSERT(swap != NULL && models->trees != NULL,
			"reading species into no store");
	models->trees_before = models->trees;
	models->trees = swap;
	trees = models->trees;
	voe_editor_trees_read(trees, folder, scratch);
	for (uint32_t i = 0; i < trees->count; i++) {
		const voe_game_tree *old = voe_editor_trees_find(
			models->trees_before, trees->rows[i].path);

		trees->regrow[i] =
			old == NULL ||
			memcmp(&old->values, &trees->rows[i].values,
			       sizeof old->values) != 0 ||
			models->trees_before
				->regrow[old - models->trees_before->rows];
	}
	VOE_BASE_ASSERT(trees->count <= VOE_EDITOR_TREES,
			"a table past its room");
}

// Empties the store, keeps a copy of `folder` (NULL for untitled) and reads
// its materials, parts and species, every one marked, when it is not the one
// last read against.
static void follow_folder(voe_editor_models *models, const char *folder,
			  voe_render_device *device, voe_base_arena *scratch)
{
	if (folder == NULL && models->folder == NULL)
		return;
	if (folder != NULL && models->folder != NULL &&
	    strcmp(folder, models->folder) == 0)
		return;
	job_cancel(models);
	voe_3d_models_clear(models->store, device);
	voe_game_compressing_clear(models->compressing, device);
	voe_editor_map_watch_clear(&models->maps);
	free(models->folder);
	models->folder = NULL;
	free(models->resized);
	models->resized = NULL;
	models->reloads = 0;
	takes_clear(models);
	if (folder != NULL) {
		size_t size = strlen(folder) + 1;

		models->folder = malloc(size);
		VOE_BASE_ASSERT(models->folder != NULL,
				"out of memory copying the models' folder");
		memcpy(models->folder, folder, size);
	}
	voe_editor_materials_read(models->materials, models->folder, scratch);
	voe_editor_parts_read(models->parts, models->folder, scratch);
	// Emptied first, since the read compares against it: every row new.
	models->trees->count = 0;
	trees_read(models, models->folder, scratch);
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

// Under a splash every species grown at once, the job let go first, when any
// row is marked; a project's first update has them all marked. The marks are
// cleared unless a stop was asked, so a stopped grow is the job's to finish.
static void trees_grow_whole(voe_editor_models *models,
			     voe_render_device *device, voe_base_arena *scratch,
			     voe_game_progress *progress,
			     voe_editor_notice *notice)
{
	voe_editor_trees *trees = models->trees;
	voe_game_trees table = voe_editor_trees_table(trees);
	bool marked = false;

	VOE_BASE_ASSERT(progress != NULL, "growing whole outside a splash");
	VOE_BASE_ASSERT(device != NULL && scratch != NULL && notice != NULL,
			"growing species without device, scratch or notice");
	for (uint32_t i = 0; i < trees->count; i++)
		marked = marked || trees->regrow[i];
	if (!marked)
		return;
	job_cancel(models);
	voe_base_report_error_clear();
	say_failure(notice, voe_game_trees_grow(models->store, device, &table,
						scratch, progress));
	if (voe_game_progress_stopped(progress))
		return;
	for (uint32_t i = 0; i < trees->count; i++)
		trees->regrow[i] = false;
}

// Outside a splash, one species at a time (0386): the first marked row started
// while the job is idle, its mark cleared so a change meanwhile marks it
// again; once its burst has ended, one variation put an update. A failed put
// is said in `notice` under the species' path.
static void trees_grow_step(voe_editor_models *models,
			    voe_render_device *device,
			    voe_editor_notice *notice)
{
	voe_editor_trees *trees = models->trees;
	voe_base_error error = VOE_BASE_OK;
	bool finished;

	VOE_BASE_ASSERT(device != NULL && notice != NULL,
			"growing a species without device or notice");
	for (uint32_t i = 0; i < trees->count && models->growing[0] == '\0';
	     i++) {
		bool started;

		if (!trees->regrow[i])
			continue;
		started = voe_game_trees_job_start(models->job, &trees->rows[i]);
		VOE_BASE_ASSERT(started, "an idle trees job refused a start");
		trees->regrow[i] = false;
		snprintf(models->growing, sizeof models->growing, "%s",
			 trees->rows[i].path);
	}
	if (models->growing[0] == '\0' || !voe_game_trees_job_done(models->job))
		return;
	voe_base_report_error_clear();
	finished = voe_game_trees_job_put(models->job, models->store, device,
					  &error);
	if (error != VOE_BASE_OK)
		say_failure(notice, (voe_game_models_failures){
					    .count = 1, .first = models->growing });
	if (finished)
		models->growing[0] = '\0';
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
			"reloading a material without device, scratch or notice");
	for (uint32_t i = 0; i < models->reloads; i++) {
		const voe_game_material *row = voe_editor_materials_find(
			models->materials, models->reload[i]);
		voe_base_error error = VOE_BASE_OK;

		if (row == NULL)
			continue;
		voe_base_report_error_clear();
		if (!voe_game_models_material_load(models->store, device,
						   models->folder, row, 0,
						   scratch, models->compressing,
						   &error))
			voe_editor_notice_set(notice, "Could not read %s: %s",
					      row->path,
					      voe_base_error_string(error));
	}
	models->reloads = 0;
}

// Every held material whose row names `map` in any of its four map slots
// waits to be loaded again, from the copy just written or the map changed on
// disk. A row the store does not hold loads when first named.
static void reload_naming(voe_editor_models *models, const char *map)
{
	VOE_BASE_ASSERT(models != NULL && models->materials != NULL,
			"reloading for a map in no store");
	VOE_BASE_ASSERT(map != NULL && map[0] != '\0', "reloading for no map");
	for (uint32_t i = 0; i < models->materials->count; i++) {
		const voe_game_material *row = &models->materials->rows[i];
		const voe_assets_material_file *values = &row->values;

		if ((strcmp(values->colormap, map) == 0 ||
		     strcmp(values->normalmap, map) == 0 ||
		     strcmp(values->ormmap, map) == 0 ||
		     strcmp(values->emissionmap, map) == 0) &&
		    voe_3d_models_find(models->store, row->path) != NULL)
			reload_add(models, row->path);
	}
}

void voe_editor_models_update(voe_editor_models *models,
			      voe_editor_session *session,
			      voe_render_device *device,
			      voe_base_arena *scratch, double now,
			      voe_game_progress *progress)
{
	voe_game_materials table;
	voe_game_parts parts;

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
	for (uint32_t i = 0; i < models->takes; i++)
		take_load(models->store, device, &models->take[i]);
	takes_clear(models);
	voe_base_report_error_clear();
	say_failure(&session->notice,
		    voe_game_models_update(session->project->world,
					   models->store, device,
					   models->folder, scratch, progress));
	table = voe_editor_materials_table(models->materials);
	// The watch before the materials, so a model it reloads is given its
	// parts before it is drawn, and a changed map's rows load this update.
	if (now - models->looked >= WATCH_SECONDS) {
		voe_editor_map_changes changed;
		uint32_t count;

		models->looked = now;
		voe_base_report_error_clear();
		say_failure(&session->notice,
			    voe_game_models_watch(models->store, device,
						  models->folder, scratch));
		count = voe_editor_map_watch_look(&models->maps, models->folder,
						  table, &changed, scratch);
		for (uint32_t i = 0; i < count; i++)
			reload_naming(models, changed.paths[i]);
	}
	if (progress != NULL)
		trees_grow_whole(models, device, scratch, progress,
				 &session->notice);
	else
		trees_grow_step(models, device, &session->notice);
	parts = voe_editor_parts_table(models->parts);
	voe_base_report_error_clear();
	// Under a splash a map is compressed inline and shows as progress; the
	// queue is for the frames outside one.
	say_failure(&session->notice,
		    voe_game_models_materials(
			    session->project->world, models->store, device,
			    models->folder, &table, &parts, scratch, progress,
			    progress == NULL ? models->compressing : NULL));
	material_reload(models, device, scratch, &session->notice);
	if (progress == NULL) {
		const char *written = voe_game_compressing_step(
			models->compressing, device, scratch);

		if (written != NULL)
			reload_naming(models, written);
	}
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
			"reloading a model without device, path or scratch");
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
	VOE_BASE_ASSERT(device != NULL, "writing landscapes without a device");
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
			    voe_base_arena *scratch, voe_editor_disk_watch *watch,
			    voe_editor_notice *why)
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
		const char *path;
		bool written;

		if (!edited_landscape(entry))
			continue;
		mark = voe_base_arena_mark(scratch);
		text = voe_assets_landscape_write(entry->landscape, scratch);
		path = voe_platform_path_join(scratch, folder, entry->path);
		written = voe_platform_file_write(path, (const uint8_t *)text.text,
						  text.size, &error);
		if (written && watch != NULL)
			voe_editor_disk_watch_wrote(watch, path);
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

// `take`'s copy loaded over its entry at the entry's own stamp, since a
// landscape is never re-read by stamp; a refused load says nothing here, as a
// re-read's does not. Nothing when the entry went, by a rename, in the meantime.
static void take_load(voe_3d_models *store, voe_render_device *device,
		      const struct landscape_take *take)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(store, take->path);
	voe_base_error error = VOE_BASE_OK;

	VOE_BASE_ASSERT(device != NULL && take->landscape != NULL,
			"taking a landscape in with no device or no copy");
	if (entry == NULL)
		return;
	voe_base_report_subject_push(take->path);
	if (voe_3d_models_load_landscape(store, device, take->path, entry->stamp,
					 take->landscape, &error)) {
		if (take->edited)
			voe_3d_models_landscape_changed(store, take->path);
	} else {
		VOE_BASE_ASSERT(error != VOE_BASE_OK,
				"a refused landscape take with no error");
	}
	voe_base_report_subject_pop();
}

void voe_editor_models_landscape_take(voe_editor_models *models,
				      const char *path,
				      const voe_assets_landscape *landscape,
				      bool edited)
{
	struct landscape_take *take = NULL;
	size_t length;

	VOE_BASE_ASSERT(models != NULL && path != NULL && landscape != NULL,
			"taking no landscape, at no path or into no store");
	if (voe_3d_models_find(models->store, path) == NULL)
		return;
	for (uint32_t i = 0; i < models->takes; i++)
		if (strcmp(models->take[i].path, path) == 0)
			take = &models->take[i];
	if (take != NULL) {
		voe_base_arena_destroy(take->arena);
	} else if (models->takes == LANDSCAPE_TAKES) {
		VOE_BASE_ERROR("editor", "left %s as it was: %u landscapes wait",
			       path, LANDSCAPE_TAKES);
		return;
	} else {
		take = &models->take[models->takes++];
	}
	length = strlen(path) + 1;
	take->arena = voe_base_arena_new(LANDSCAPE_TAKE_ARENA);
	take->path = memcpy(voe_base_arena_push(take->arena, length), path,
			    length);
	take->landscape = voe_base_arena_push(take->arena,
					      sizeof *take->landscape);
	*take->landscape = voe_assets_landscape_resample(
		landscape, landscape->cells, take->arena);
	take->edited = edited;
	VOE_BASE_ASSERT(models->takes <= LANDSCAPE_TAKES,
			"more takes waiting than their room");
}

const voe_assets_landscape *
voe_editor_models_landscape_held(const voe_editor_models *models,
				 const char *path)
{
	const voe_3d_model_entry *entry;

	VOE_BASE_ASSERT(models != NULL && models->store != NULL,
			"a landscape held by no store");
	VOE_BASE_ASSERT(path != NULL, "the landscape held at no path");
	entry = voe_3d_models_find(models->store, path);
	return entry != NULL ? entry->landscape : NULL;
}

bool voe_editor_models_landscape_edited(const voe_editor_models *models,
					const char *path)
{
	const voe_3d_model_entry *entry;

	VOE_BASE_ASSERT(models != NULL && models->store != NULL,
			"asking whether a landscape is edited in no store");
	VOE_BASE_ASSERT(path != NULL, "asking whether no landscape is edited");
	entry = voe_3d_models_find(models->store, path);
	return entry != NULL && edited_landscape(entry);
}

void voe_editor_models_revert(voe_editor_models *models, const char *folder,
			      voe_render_device *device,
			      voe_base_arena *scratch)
{
	uint32_t count;

	VOE_BASE_ASSERT(models != NULL && models->store != NULL,
			"reverting landscapes of no store");
	VOE_BASE_ASSERT(device != NULL && scratch != NULL,
			"reverting landscapes without device or scratch");
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
	trees_read(models, folder, scratch);
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

void voe_editor_models_tree_changed(voe_editor_models *models,
				    const char *path,
				    const voe_assets_tree *values)
{
	voe_game_tree *row;

	VOE_BASE_ASSERT(models != NULL && models->trees != NULL &&
				path != NULL && values != NULL,
			"no species changed, or in no store");
	VOE_BASE_ASSERT(values->variations >= VOE_ASSETS_TREE_VARIATIONS_MIN &&
				values->variations <=
					VOE_ASSETS_TREE_VARIATIONS_MAX,
			"a species of 1 to 16 variations");
	row = voe_editor_trees_find(models->trees, path);
	if (row == NULL)
		return;
	row->values = *values;
	models->trees->regrow[row - models->trees->rows] = true;
}

const voe_editor_trees *voe_editor_models_trees(const voe_editor_models *models)
{
	VOE_BASE_ASSERT(models != NULL, "the species of no store");
	VOE_BASE_ASSERT(models->trees != NULL, "a store with no species");
	return models->trees;
}

const voe_3d_models *voe_editor_models_store(const voe_editor_models *models)
{
	VOE_BASE_ASSERT(models != NULL, "reading no model store");
	VOE_BASE_ASSERT(models->store != NULL, "a model store with none");
	return models->store;
}
