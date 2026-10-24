// The batch of game/src/models_batch.h: its files' phases as tasks run in
// order on the caller, their put one at a time, the times and their line; and
// the levels file of a `.glb`, read on its arena or made and written, which
// the public prepare shares. A file's state moves only forward, set by the
// task of its phase or between phases: a picture's failure is its own until
// the decoding phase is done, then folded into its file's. Every arena is
// made with voe_base_arena_new and destroyed after the file's put or by
// destroy, the batch's own (its files, paths and pictures) last. A `.glb`'s
// cooked copy is taken in reading onto its file's arena and written in cooking
// from that arena, as the batch's mode says (models_cooked.h); the progress,
// when given, shows each file's phase from its reading on. A `.glb`'s levels file is
// `<folder>/Cache/levels/<path>.levels`, its folders made first when absent,
// with the `[Levels]` of the `.parts` beside the `.glb`, the defaults when
// there is none or it will not parse; a fresh file kept whose swaps differ
// from those of a `.parts` that reads has them patched into a copy, written
// back and handed on, not made again; with no `.parts` its own swaps stand.
#include "models_batch.h"
#include "models_cooked.h"

#include <3d/model_levels.h>
#include <3d/model_prepare.h>

#include <assets/model.h>
#include <assets/parts.h>

#include <base/assert.h>
#include <base/report.h>

#include <platform/clock.h>
#include <platform/file.h>
#include <platform/path.h>

#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

// Block sizes, not limits: a push past one gets a block of its own.
#define BATCH_BLOCK (16 * 1024)
#define FILE_BLOCK (1024 * 1024)
#define PICTURE_BLOCK (1024 * 1024)

// Where a file stands; each phase moves only those it reads on.
enum file_state {
	FILE_ADDED,
	// No stamp: kept failed at 0 when known at another or not held.
	FILE_UNSTAMPED,
	// At its known stamp and not forced: nothing to do.
	FILE_KNOWN,
	FILE_UNREADABLE,
	// Bytes read: a `.glb` parsed and levelled on, any other loaded whole.
	FILE_READ,
	// A `.glb` its parse, levels make, a picture or bake refused.
	FILE_REFUSED,
	FILE_PREPARED,
};

struct batch_file {
	const char *path;
	const char *full;
	uint64_t known;
	bool forced;
	bool glb;
	enum file_state state;
	uint64_t stamp;
	voe_base_error error;
	// The file's own, NULL until read and after its put.
	voe_base_arena *arena;
	const uint8_t *bytes;
	size_t size;
	voe_assets_model model;
	// model.images, written by the decoding tasks.
	voe_assets_image *images;
	const uint8_t *levels;
	size_t levels_size;
	const voe_3d_model_prepared *prepared;
	// Its pictures in the batch's list.
	uint32_t first_picture;
	uint32_t picture_count;
	// A `.glb`'s copy: its key, whether one was taken or written, and the
	// phase the progress shows for the file.
	voe_3d_model_cooked_key key;
	bool taken;
	bool cooked;
	const char *phase;
};

// One picture of one file, decoded on an arena of its own.
struct batch_picture {
	uint32_t file;
	uint32_t index;
	voe_base_arena *arena;
	voe_base_error error;
};

struct voe_game_models_batch {
	voe_base_arena *arena;
	const char *folder;
	struct batch_file *files;
	uint32_t count;
	uint32_t room;
	struct batch_picture *pictures;
	uint32_t picture_count;
	enum voe_game_models_cooking cooking;
	voe_game_progress *progress;
};

void voe_game_models_failure_count(voe_game_models_failures *failures,
				   const voe_3d_models *models,
				   const char *path)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(models, path);

	VOE_BASE_ASSERT(failures != NULL, "counting into no failures");
	VOE_BASE_ASSERT(path != NULL, "counting a failure with no path");
	// The dot keeps no failed entry; its "" is a literal, valid for ever.
	if (failures->count == 0 && entry != NULL)
		failures->first = entry->path;
	else if (failures->count == 0 && path[0] == '\0')
		failures->first = "";
	failures->count++;
}

// The one line of levels made or patched whose write failed with `error`,
// under the file's subject.
static void report_unsaved(voe_base_error error)
{
	VOE_BASE_ERROR("game",
		       "the LODs could not be saved in the project's "
		       "`Cache/levels/`: %s",
		       voe_base_error_string(error));
}

// The `[Levels]` of the `.parts` beside `folder`'s `.glb` `path` (0430 point
// 5), `*parted` whether that file was there and read: the defaults, silently,
// when there is no such file, and when it will not parse, the parts reader
// having said why. Scratch rewound.
static voe_assets_levels levels_asked(const char *folder, const char *path,
				      voe_base_arena *scratch, bool *parted)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	const char *file = voe_game_models_parts_file(folder, path, scratch);
	voe_assets_levels levels = VOE_ASSETS_LEVELS_DEFAULT;
	voe_assets_parts parts;
	const uint8_t *text = NULL;
	size_t size = 0;

	VOE_BASE_ASSERT(parted != NULL, "no parted");
	if (voe_platform_file_exists(file))
		text = voe_platform_file_read(file, scratch, &size, NULL);
	*parted = text != NULL &&
		  voe_assets_parts_read((const char *)text, size, &parts, NULL);
	if (*parted)
		levels = parts.levels;
	voe_base_arena_rewind(scratch, mark);
	return levels;
}

const char *voe_game_models_levels_file(const char *levels, const char *path,
					voe_base_arena *scratch)
{
	size_t length = strlen(path);
	char *name = voe_base_arena_push(scratch, length + sizeof(".levels"));

	VOE_BASE_ASSERT(levels != NULL, "no levels folder");
	VOE_BASE_ASSERT(name != NULL, "no room for a path");
	memcpy(name, path, length);
	memcpy(name + length, ".levels", sizeof(".levels"));
	return voe_platform_path_join(scratch, levels, name);
}

// The fresh levels file `bytes`, whose swaps `read` holds, handed on as it is
// unless `asked`'s swaps 1 to 5 differ (0436 point 7): then a copy on scratch
// with them set, written back to `file`, a failed write a line on stderr.
static const uint8_t *swaps_patched(const char *file, const uint8_t *bytes,
				    size_t size,
				    const voe_3d_model_levels *read,
				    const voe_assets_levels *asked,
				    voe_base_arena *scratch)
{
	voe_base_error unsaved = VOE_BASE_OK;
	bool differ = false;
	uint8_t *copy;

	VOE_BASE_ASSERT(bytes != NULL && read != NULL && asked != NULL,
			"patching no file");
	for (uint32_t k = 1; k < VOE_3D_MODEL_LEVELS; k++)
		differ = differ || read->swaps[k] != asked->swaps[k];
	if (!differ)
		return bytes;
	copy = voe_base_arena_push(scratch, size);
	VOE_BASE_ASSERT(copy != NULL, "no room for the levels");
	memcpy(copy, bytes, size);
	voe_3d_model_levels_swaps_set(copy, size, asked->swaps);
	if (!voe_platform_file_write(file, copy, size, &unsaved))
		report_unsaved(unsaved);
	return copy;
}

const uint8_t *voe_game_models_levels_for(const char *folder, const char *path,
					  const uint8_t *glb, size_t glb_size,
					  bool remake, voe_base_arena *scratch,
					  size_t *size, bool *made,
					  voe_base_error *error)
{
	const char *levels = voe_platform_path_join(scratch, folder,
						    "Cache/levels");
	const char *file = voe_game_models_levels_file(levels, path, scratch);
	const uint8_t *bytes = NULL;
	voe_base_error unsaved = VOE_BASE_OK;
	voe_3d_model_levels read;
	bool parted;
	voe_assets_levels asked = levels_asked(folder, path, scratch, &parted);

	VOE_BASE_ASSERT(glb != NULL && size != NULL, "no `.glb` or size");
	VOE_BASE_ASSERT(made != NULL && error != NULL, "no made or error");
	*made = false;
	if (!remake && voe_platform_file_exists(file))
		bytes = voe_platform_file_read(file, scratch, size, NULL);
	// A shipped game has no `.parts`: the file's own swaps stand.
	if (bytes != NULL && voe_3d_model_levels_read(scratch, bytes, *size,
						      glb, glb_size, &read))
		return parted ? swaps_patched(file, bytes, *size, &read, &asked,
					      scratch) :
				bytes;
	bytes = voe_3d_model_levels_make(scratch, glb, glb_size, asked, size,
					 error);
	if (bytes == NULL)
		return NULL;
	*made = true;
	if (!voe_game_models_folders_make(folder, levels, path, scratch,
					  &unsaved) ||
	    !voe_platform_file_write(file, bytes, *size, &unsaved))
		report_unsaved(unsaved);
	return bytes;
}

voe_game_models_batch *
voe_game_models_batch_new(const char *folder, uint32_t room,
			  enum voe_game_models_cooking cooking,
			  voe_game_progress *progress)
{
	voe_base_arena *arena = voe_base_arena_new(BATCH_BLOCK);
	voe_game_models_batch *batch = voe_base_arena_push(arena, sizeof(*batch));

	VOE_BASE_ASSERT(folder != NULL, "a batch of no folder");
	VOE_BASE_ASSERT(room > 0, "a batch with no room");
	*batch = (voe_game_models_batch){
		.arena = arena,
		.folder = folder,
		.files = voe_base_arena_push(arena, room * sizeof(*batch->files)),
		.room = room,
		.cooking = cooking,
		.progress = progress,
	};
	return batch;
}

// File `index`'s arena and its pictures' given back; again is nothing.
static void file_release(voe_game_models_batch *batch, uint32_t index)
{
	struct batch_file *file = &batch->files[index];

	VOE_BASE_ASSERT(index < batch->count, "releasing a file past the batch");
	for (uint32_t k = 0; k < file->picture_count; k++) {
		struct batch_picture *picture =
			&batch->pictures[file->first_picture + k];

		if (picture->arena != NULL)
			voe_base_arena_destroy(picture->arena);
		picture->arena = NULL;
	}
	file->picture_count = 0;
	if (file->arena != NULL)
		voe_base_arena_destroy(file->arena);
	file->arena = NULL;
	VOE_BASE_ASSERT(file->arena == NULL, "a file's arena kept");
}

void voe_game_models_batch_destroy(voe_game_models_batch *batch)
{
	VOE_BASE_ASSERT(batch != NULL, "destroying no batch");
	VOE_BASE_ASSERT(batch->count <= batch->room, "a batch past its room");
	for (uint32_t i = 0; i < batch->count; i++)
		file_release(batch, i);
	voe_base_arena_destroy(batch->arena);
}

void voe_game_models_batch_add(voe_game_models_batch *batch, const char *path,
			       uint64_t known, bool forced)
{
	size_t length;
	char *copy;

	VOE_BASE_ASSERT(batch != NULL && batch->count < batch->room,
			"adding past a batch's room");
	VOE_BASE_ASSERT(path != NULL && path[0] != '\0',
			"loading a model with no path");
	length = strlen(path) + 1;
	copy = voe_base_arena_push(batch->arena, length);
	memcpy(copy, path, length);
	batch->files[batch->count++] = (struct batch_file){
		.path = copy,
		.full = voe_platform_path_join(batch->arena, batch->folder,
					       path),
		.known = known,
		.forced = forced,
		.glb = voe_game_models_is_glb(path),
		.state = FILE_ADDED,
		.error = VOE_BASE_OK,
		.phase = "Loading models",
	};
}

uint32_t voe_game_models_batch_count(const voe_game_models_batch *batch)
{
	VOE_BASE_ASSERT(batch != NULL, "counting no batch");
	VOE_BASE_ASSERT(batch->count <= batch->room, "a batch past its room");
	return batch->count;
}

// A `.glb`'s copy, on its arena, taken as the batch's mode allows, prepared
// then; the file's phase "Preparing models" when it will be cooked instead,
// and shown, its counts kept.
static void copy_take(voe_game_models_batch *batch, struct batch_file *file)
{
	enum voe_game_models_cooking cooking = batch->cooking;
	voe_game_progress *progress = batch->progress;

	file->key = voe_game_models_cooked_key(batch->folder, file->path,
					       file->stamp, file->arena);
	if (cooking != VOE_GAME_MODELS_COOK_REMAKE)
		file->prepared = voe_game_models_cooked_take(
			batch->folder, file->path, file->key,
			cooking != VOE_GAME_MODELS_COOK_MISSING, file->arena);
	file->taken = file->prepared != NULL;
	if (file->taken)
		file->state = FILE_PREPARED;
	else if (cooking != VOE_GAME_MODELS_COOK_NEVER)
		file->phase = "Preparing models";
	if (progress != NULL)
		voe_game_progress_set(progress, file->phase,
				      atomic_load(&progress->done),
				      atomic_load(&progress->total));
}

// Reading: the stamp, then, unless the stamp is the known one and the file
// not forced, a `.glb`'s copy or the bytes on the file's own arena.
static void read_task(voe_game_models_batch *batch, uint32_t index)
{
	struct batch_file *file = &batch->files[index];

	VOE_BASE_ASSERT(index < batch->count, "reading a file past the batch");
	VOE_BASE_ASSERT(file->state == FILE_ADDED, "reading a file twice");
	voe_base_report_subject_push(file->path);
	if (!voe_platform_file_stamp(file->full, &file->stamp)) {
		file->state = FILE_UNSTAMPED;
		file->error = VOE_BASE_ERROR_UNAVAILABLE;
	} else if (!file->forced && file->stamp == file->known) {
		file->state = FILE_KNOWN;
	} else {
		file->arena = voe_base_arena_new(FILE_BLOCK);
		if (file->glb)
			copy_take(batch, file);
	}
	if (file->state == FILE_ADDED) {
		file->bytes = voe_platform_file_read(file->full, file->arena,
						     &file->size, &file->error);
		file->state = file->bytes != NULL ? FILE_READ : FILE_UNREADABLE;
	}
	voe_base_report_subject_pop();
}

// A read `.glb`'s file, the one the parsing, LODs and baking phases work on.
static struct batch_file *glb_at(voe_game_models_batch *batch, uint32_t index)
{
	struct batch_file *file = &batch->files[index];

	VOE_BASE_ASSERT(index < batch->count, "a file past the batch");
	VOE_BASE_ASSERT(file->state != FILE_ADDED, "a file not yet read");
	return file->state == FILE_READ && file->glb ? file : NULL;
}

// Parsing: the `.glb` with its pictures left encoded, and room for them.
static void parse_task(voe_game_models_batch *batch, uint32_t index)
{
	struct batch_file *file = glb_at(batch, index);

	if (file == NULL)
		return;
	voe_base_report_subject_push(file->path);
	if (!voe_assets_model_read_glb_encoded(file->bytes, file->size,
					       file->arena, &file->model,
					       &file->error)) {
		file->state = FILE_REFUSED;
	} else if (file->model.image_count > 0) {
		file->images = voe_base_arena_push(
			file->arena,
			file->model.image_count * sizeof(*file->images));
		file->model.images = file->images;
	}
	voe_base_report_subject_pop();
	VOE_BASE_ASSERT(file->state == FILE_READ || file->state == FILE_REFUSED,
			"a parse left its file elsewhere");
}

// Making LODs: the levels file read, or made and written.
static void lods_task(voe_game_models_batch *batch, uint32_t index)
{
	struct batch_file *file = glb_at(batch, index);
	bool made;

	if (file == NULL)
		return;
	voe_base_report_subject_push(file->path);
	file->levels = voe_game_models_levels_for(
		batch->folder, file->path, file->bytes, file->size, false,
		file->arena, &file->levels_size, &made, &file->error);
	// Refused, it is kept failed at its stamp, not read again (0445 point 4).
	if (file->levels == NULL)
		file->state = FILE_REFUSED;
	voe_base_report_subject_pop();
}

// Every picture of the batch's `.glb`s still going into the batch's list; a
// write never passes the counted list.
static void pictures_gather(voe_game_models_batch *batch)
{
	uint32_t total = 0;
	uint32_t at = 0;

	for (uint32_t i = 0; i < batch->count; i++)
		if (glb_at(batch, i) != NULL)
			total += batch->files[i].model.image_count;
	batch->pictures = total == 0 ? NULL :
		voe_base_arena_push(batch->arena,
				    total * sizeof(*batch->pictures));
	for (uint32_t i = 0; i < batch->count; i++) {
		struct batch_file *file = glb_at(batch, i);

		if (file == NULL)
			continue;
		file->first_picture = at;
		file->picture_count = file->model.image_count;
		for (uint32_t k = 0; k < file->picture_count && at < total; k++)
			batch->pictures[at++] = (struct batch_picture){
				.file = i, .index = k, .error = VOE_BASE_OK
			};
	}
	batch->picture_count = total;
	VOE_BASE_ASSERT(at == total, "pictures gathered miscounted");
}

// Decoding pictures: one, on an arena of its own, into its file's images.
static void decode_task(voe_game_models_batch *batch, uint32_t index)
{
	struct batch_picture *picture = &batch->pictures[index];
	struct batch_file *file = &batch->files[picture->file];

	VOE_BASE_ASSERT(index < batch->picture_count, "a picture past the batch");
	VOE_BASE_ASSERT(file->images != NULL, "a picture with no room");
	voe_base_report_subject_push(file->path);
	picture->arena = voe_base_arena_new(PICTURE_BLOCK);
	if (!voe_assets_model_decode_picture(&file->model, picture->index,
					     picture->arena,
					     &file->images[picture->index],
					     &picture->error))
		VOE_BASE_ASSERT(picture->error != VOE_BASE_OK,
				"a refused picture with no category");
	voe_base_report_subject_pop();
}

// A refused picture refuses its file, the first one's category kept.
static void pictures_fold(voe_game_models_batch *batch)
{
	for (uint32_t p = 0; p < batch->picture_count; p++) {
		const struct batch_picture *picture = &batch->pictures[p];
		struct batch_file *file = &batch->files[picture->file];

		if (picture->error == VOE_BASE_OK || file->state == FILE_REFUSED)
			continue;
		file->state = FILE_REFUSED;
		file->error = picture->error;
	}
	VOE_BASE_ASSERT(batch->picture_count == 0 || batch->pictures != NULL,
			"pictures counted with no list");
}

// Baking: 3d's prepare, on the file's arena.
static void bake_task(voe_game_models_batch *batch, uint32_t index)
{
	struct batch_file *file = glb_at(batch, index);

	if (file == NULL)
		return;
	voe_base_report_subject_push(file->path);
	file->prepared = voe_3d_model_prepare(file->arena, &file->model,
					      file->bytes, file->size,
					      file->levels, file->levels_size,
					      &file->error);
	file->state = file->prepared != NULL ? FILE_PREPARED : FILE_REFUSED;
	voe_base_report_subject_pop();
}

// Cooking: a `.glb` baked here, its pictures still decoded, written as its
// copy on its arena, unless the batch's mode writes none. The write compresses
// every picture, minutes for a large one, so the file's phase is "Compressing
// textures" first, and shown, its counts kept.
static void cook_task(voe_game_models_batch *batch, uint32_t index)
{
	struct batch_file *file = &batch->files[index];
	voe_game_progress *progress = batch->progress;

	VOE_BASE_ASSERT(index < batch->count, "cooking a file past the batch");
	if (batch->cooking == VOE_GAME_MODELS_COOK_NEVER ||
	    file->state != FILE_PREPARED || file->taken)
		return;
	VOE_BASE_ASSERT(file->glb, "cooking a file that is no `.glb`");
	file->phase = "Compressing textures";
	if (progress != NULL)
		voe_game_progress_set(progress, file->phase,
				      atomic_load(&progress->done),
				      atomic_load(&progress->total));
	voe_base_report_subject_push(file->path);
	voe_game_models_cooked_save(batch->folder, file->path, file->prepared,
				    file->key, file->arena);
	file->cooked = true;
	voe_base_report_subject_pop();
}

void voe_game_models_batch_prepare(voe_game_models_batch *batch,
				   voe_game_models_times *times)
{
	static void (*const phases[VOE_GAME_MODELS_PHASES])(
		voe_game_models_batch *, uint32_t) = {
		read_task, parse_task, lods_task, decode_task, bake_task,
		NULL, cook_task
	};

	VOE_BASE_ASSERT(batch != NULL && times != NULL, "no batch or times");
	VOE_BASE_ASSERT(batch->count <= batch->room, "a batch past its room");
	for (uint32_t p = 0; p < VOE_GAME_MODELS_PHASES; p++) {
		double before = voe_platform_clock_now();
		bool decoding = p == VOE_GAME_MODELS_DECODING;
		uint32_t tasks;

		// Uploading is the put's.
		if (phases[p] == NULL)
			continue;
		if (decoding)
			pictures_gather(batch);
		tasks = decoding ? batch->picture_count : batch->count;
		for (uint32_t i = 0; i < tasks; i++)
			phases[p](batch, i);
		if (decoding)
			pictures_fold(batch);
		times->seconds[p] += voe_platform_clock_now() - before;
	}
	for (uint32_t i = 0; i < batch->count; i++)
		times->files += batch->files[i].bytes != NULL ||
				batch->files[i].taken;
}

bool voe_game_models_batch_cooked(const voe_game_models_batch *batch,
				  uint32_t index)
{
	VOE_BASE_ASSERT(batch != NULL && index < batch->count,
			"asking after a file past the batch");
	return batch->files[index].cooked;
}

const char *voe_game_models_batch_phase(const voe_game_models_batch *batch,
					uint32_t index)
{
	VOE_BASE_ASSERT(batch != NULL && index < batch->count,
			"asking after a file past the batch");
	return batch->files[index].phase;
}

// The file into the store as its state says, under its path as the subject;
// whether it failed, its line said here or where its cause was met.
static bool file_put(struct batch_file *file, voe_3d_models *models,
		     voe_render_device *device, bool held)
{
	bool unread = file->state == FILE_UNREADABLE ||
		      (file->state == FILE_UNSTAMPED &&
		       (file->known != 0 || !held));

	VOE_BASE_ASSERT(file->state != FILE_ADDED, "putting a file not read");
	VOE_BASE_ASSERT(file->state != FILE_READ || !file->glb,
			"putting a `.glb` not prepared");
	if (unread) {
		voe_3d_models_fail(models, file->path, 0);
		VOE_BASE_ERROR("game", "the file could not be read: %s",
			       voe_base_error_string(file->error));
		return true;
	}
	if (file->state == FILE_REFUSED) {
		voe_3d_models_fail(models, file->path, file->stamp);
		return true;
	}
	if (file->state == FILE_PREPARED)
		return !voe_3d_models_put(models, device, file->path,
					  file->stamp, file->prepared,
					  &file->error);
	if (file->state == FILE_READ)
		return !voe_3d_models_load(models, device, file->path,
					   file->stamp, file->bytes, file->size,
					   &file->error);
	return false;
}

bool voe_game_models_batch_put(voe_game_models_batch *batch,
			       voe_3d_models *models, voe_render_device *device,
			       uint32_t index,
			       voe_game_models_failures *failures,
			       voe_base_error *error,
			       voe_game_models_times *times)
{
	double before = voe_platform_clock_now();
	struct batch_file *file;
	bool held;
	bool failed;

	VOE_BASE_ASSERT(batch != NULL && index < batch->count,
			"putting a file past the batch");
	VOE_BASE_ASSERT(models != NULL && device != NULL && failures != NULL &&
				error != NULL && times != NULL,
			"no store, device, failures, error or times");
	file = &batch->files[index];
	held = voe_3d_models_find(models, file->path) != NULL;
	*error = VOE_BASE_OK;
	if (!held && voe_3d_models_count(models) == VOE_3D_MODELS)
		return false;
	voe_base_report_subject_push(file->path);
	failed = file_put(file, models, device, held);
	voe_base_report_subject_pop();
	if (failed) {
		*error = file->error;
		voe_game_models_failure_count(failures, models, file->path);
	}
	file_release(batch, index);
	times->seconds[VOE_GAME_MODELS_UPLOADING] +=
		voe_platform_clock_now() - before;
	VOE_BASE_ASSERT(!failed || *error != VOE_BASE_OK,
			"a failed model load with no category");
	return true;
}

bool voe_game_models_batch_held(const voe_game_models_batch *batch,
				const voe_3d_models *models, uint32_t index)
{
	const struct batch_file *file;
	const voe_3d_model_entry *entry;

	VOE_BASE_ASSERT(batch != NULL && models != NULL && index < batch->count,
			"asking after a file past the batch");
	file = &batch->files[index];
	VOE_BASE_ASSERT(file->state != FILE_ADDED, "asking after a file not read");
	entry = voe_3d_models_find(models, file->path);
	return entry != NULL &&
	       entry->stamp == (file->state == FILE_UNSTAMPED ? 0 : file->stamp);
}

// The line is composed whole and written in one call, so another thread's
// line cannot land inside it.
void voe_game_models_times_write(const voe_game_models_times *times,
				 const char *noun, uint32_t line)
{
	static const char *const names[VOE_GAME_MODELS_PHASES] = {
		"reading", "parsing", "making LODs", "decoding pictures",
		"baking", "uploading", "cooking"
	};
	char phases[256];
	size_t at = 0;
	double total = 0.0;

	VOE_BASE_ASSERT(times != NULL && noun != NULL, "writing no times");
	VOE_BASE_ASSERT(line != 0 && line <= VOE_GAME_MODELS_LINE,
			"a line of no phases");
	if (times->files == 0)
		return;
	phases[0] = '\0';
	for (uint32_t p = 0; p < VOE_GAME_MODELS_PHASES; p++) {
		if ((line & (1u << p)) == 0)
			continue;
		total += times->seconds[p];
		// Seven names and numbers fit; a cut line is still a line.
		at += (size_t)snprintf(phases + at, sizeof(phases) - at,
				       "%s%s %.3f s", at > 0 ? ", " : "",
				       names[p], times->seconds[p]);
		at = at < sizeof(phases) ? at : sizeof(phases) - 1;
	}
	VOE_BASE_ASSERT(total >= 0.0, "a phase took less than no time");
	// A stderr that will not take the line leaves nobody to tell.
	(void)fprintf(stderr, "game: %s: %u files in %.3f s: %s\n", noun,
		      (unsigned)times->files, total, phases);
}
