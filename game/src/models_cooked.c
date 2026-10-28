// The cooked copies of game/src/models_cooked.h: a model's and a map's copy,
// each its path, take and save, a model's key, and the folder and `.parts`
// helpers the levels files share. A take asks whether the copy is there before reading it, so a missing
// copy says nothing on stderr; a folder already there is found by a listing
// for the same reason, and one a burst's other task made meanwhile by a
// second. Every helper pushes its paths on the arena it is given.
//
// The public stale and cook of game/models.h walk one list of inputs: the
// `.glb`s of the paths given, then each table row's four maps in order. An
// input is stale when it has a stamp and its copy's head is missing, will not
// read or holds another key; the stale models are cooked by batches of
// min(4, splash workers) in prepare's STALE mode (models_batch.h), a map read,
// decoded and saved here on scratch, rewound after it. A model's save
// compresses its pictures, mipped by kind in the batch's decoding, (0477) on
// the caller's device, started and waited for (0486); a map's write is its own
// call so a chain compressed elsewhere is written the same way.
//
// Constraints: a map is compared with every map before it to count a path and
// kind once, a square in the table's maps; a sorted table would lift it.
#include "models_cooked.h"
#include "models_batch.h"

#include <game/models.h>

#include <base/assert.h>
#include <base/report.h>

#include <platform/burst.h>
#include <platform/file.h>
#include <platform/folder.h>
#include <platform/path.h>

#include <ctype.h>
#include <string.h>

// A material's four maps.
#define MAPS 4

// Each map's kind by its slot (0470 point 2), as models_materials.c mips it.
static const voe_assets_image_kind MAP_KINDS[MAPS] = {
	VOE_ASSETS_IMAGE_COLOUR, VOE_ASSETS_IMAGE_NORMAL, VOE_ASSETS_IMAGE_DATA,
	VOE_ASSETS_IMAGE_COLOUR
};

bool voe_game_models_is_glb(const char *path)
{
	static const char suffix[] = ".glb";
	size_t tail = sizeof(suffix) - 1;
	size_t length;

	VOE_BASE_ASSERT(path != NULL, "asking after no path");
	length = strlen(path);
	if (length < tail)
		return false;
	for (size_t i = 0; i < tail; i++)
		if (tolower((unsigned char)path[length - tail + i]) != suffix[i])
			return false;
	return true;
}

const char *voe_game_models_parts_file(const char *folder, const char *path,
				       voe_base_arena *arena)
{
	size_t stem = strlen(path) -
		      (voe_game_models_is_glb(path) ? sizeof(".glb") - 1 : 0);
	char *name = voe_base_arena_push(arena, stem + sizeof(".parts"));

	VOE_BASE_ASSERT(folder != NULL, "a `.parts` in no folder");
	VOE_BASE_ASSERT(name != NULL, "no room for a path");
	memcpy(name, path, stem);
	memcpy(name + stem, ".parts", sizeof(".parts"));
	return voe_platform_path_join(arena, folder, name);
}

// Whether a listing finds the folder `path`. Scratch rewound.
static bool folder_there(const char *path, voe_base_arena *scratch)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	voe_platform_folder_listing listing;
	bool there = voe_platform_folder_list(path, scratch, &listing, NULL);

	VOE_BASE_ASSERT(path != NULL, "looking for no folder");
	voe_base_arena_rewind(scratch, mark);
	return there;
}

// Makes the folder `path` unless a listing finds one there. One another
// thread's task made between the look and the make is refused as taken and
// found by a second listing: made, though platform said its line. False,
// `error` set, when it could not be made.
static bool make_absent(const char *path, voe_base_arena *scratch,
			voe_base_error *error)
{
	if (folder_there(path, scratch) ||
	    voe_platform_folder_create(path, error))
		return true;
	if (*error != VOE_BASE_ERROR_REFUSED || !folder_there(path, scratch))
		return false;
	*error = VOE_BASE_OK;
	return true;
}

bool voe_game_models_folders_make(const char *folder, const char *cache,
				  const char *path, voe_base_arena *scratch,
				  voe_base_error *error)
{
	size_t length = strlen(path);
	char *prefix = voe_base_arena_push(scratch, length + 1);
	bool made = make_absent(voe_platform_path_join(scratch, folder, "Cache"),
				scratch, error) &&
		    make_absent(cache, scratch, error);

	VOE_BASE_ASSERT(prefix != NULL, "no room for a path");
	memcpy(prefix, path, length + 1);
	for (size_t i = 0; made && i < length; i++) {
		if (path[i] != '/')
			continue;
		prefix[i] = '\0';
		made = make_absent(voe_platform_path_join(scratch, cache, prefix),
				   scratch, error);
		prefix[i] = '/';
	}
	return made;
}

// `<folder>/<cache>/<path><ending>`, on `arena`.
static const char *cache_file(const char *folder, const char *cache,
			      const char *path, const char *ending,
			      voe_base_arena *arena)
{
	size_t length = strlen(path);
	size_t tail = strlen(ending);
	char *name = voe_base_arena_push(arena, length + tail + 1);

	VOE_BASE_ASSERT(folder != NULL, "a copy in no folder");
	VOE_BASE_ASSERT(name != NULL, "no room for a path");
	memcpy(name, path, length);
	memcpy(name + length, ending, tail + 1);
	return voe_platform_path_join(
		arena, voe_platform_path_join(arena, folder, cache), name);
}

const char *voe_game_models_cooked_file(const char *folder, const char *path,
					voe_base_arena *arena)
{
	return cache_file(folder, "Cache/models", path, ".model", arena);
}

const char *voe_game_models_picture_file(const char *folder, const char *path,
					 voe_assets_image_kind kind,
					 voe_base_arena *arena)
{
	static const char *const ENDINGS[] = { ".colour", ".normal", ".data" };

	VOE_BASE_ASSERT(kind <= VOE_ASSETS_IMAGE_DATA, "a picture of no kind");
	return cache_file(folder, "Cache/pictures", path, ENDINGS[kind], arena);
}

bool voe_game_models_picture_take(const char *folder, const char *path,
				  voe_assets_image_kind kind, uint64_t stamp,
				  bool compare, voe_base_arena *arena,
				  voe_assets_image *image)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
	const char *file = voe_game_models_picture_file(folder, path, kind, arena);
	voe_assets_image taken = { 0 };
	voe_assets_image_kind kept = kind;
	const uint8_t *bytes = NULL;
	uint64_t key = 0;
	size_t size = 0;
	size_t used = 0;

	VOE_BASE_ASSERT(image != NULL, "taking a copy into no picture");
	if (voe_platform_file_exists(file))
		bytes = voe_platform_file_read(file, arena, &size, NULL);
	if (bytes != NULL &&
	    voe_assets_image_cooked_read(bytes, size, &taken, &kept, &key,
					 &used) &&
	    kept == kind && (!compare || key == stamp)) {
		*image = taken;
		return true;
	}
	voe_base_arena_rewind(arena, mark);
	return false;
}

voe_assets_image voe_game_models_compress(voe_render_device *device,
					  const voe_assets_image *chain,
					  voe_assets_image_kind kind,
					  voe_base_arena *arena)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
	voe_assets_image blocks;
	voe_base_error error = VOE_BASE_OK;
	voe_render_compressing *job;

	VOE_BASE_ASSERT(device != NULL && chain != NULL,
			"compressing on no device or no chain");
	blocks = voe_assets_image_blocks_new(arena, chain, kind);
	job = voe_render_compress_start(device,
					kind == VOE_ASSETS_IMAGE_NORMAL ?
						VOE_RENDER_BLOCKS_BC5 :
						VOE_RENDER_BLOCKS_BC7,
					chain->width, chain->height,
					chain->levels, chain->pixels, &error);
	// Render said the one line; an RGBA8 chain is a copy still (0477).
	if (job == NULL) {
		voe_base_arena_rewind(arena, mark);
		return *chain;
	}
	voe_render_compress_finish(device, job, blocks.pixels);
	return blocks;
}

voe_assets_image voe_game_models_picture_save(const char *folder,
					      const char *path,
					      const voe_assets_image *image,
					      voe_assets_image_kind kind,
					      uint64_t stamp,
					      voe_render_device *device,
					      voe_base_arena *arena)
{
	voe_assets_image chain = voe_assets_image_mips(arena, image, kind);
	voe_assets_image blocks =
		voe_game_models_compress(device, &chain, kind, arena);

	voe_game_models_picture_write(folder, path, &blocks, kind, stamp, arena);
	return blocks;
}

void voe_game_models_picture_write(const char *folder, const char *path,
				   const voe_assets_image *chain,
				   voe_assets_image_kind kind, uint64_t stamp,
				   voe_base_arena *arena)
{
	voe_assets_bytes copy =
		voe_assets_image_cooked_write(arena, chain, kind, stamp);
	const char *cache = voe_platform_path_join(arena, folder,
						   "Cache/pictures");
	voe_base_error unsaved = VOE_BASE_OK;

	VOE_BASE_ASSERT(copy.bytes != NULL && copy.count > 0, "an empty copy");
	if (!voe_game_models_folders_make(folder, cache, path, arena, &unsaved) ||
	    !voe_platform_file_write(
		    voe_game_models_picture_file(folder, path, kind, arena),
		    copy.bytes, copy.count, &unsaved))
		VOE_BASE_ERROR("game",
			       "the map's cooked copy could not be saved in the "
			       "project's `Cache/pictures/`: %s",
			       voe_base_error_string(unsaved));
}

voe_3d_model_cooked_key voe_game_models_cooked_key(const char *folder,
						   const char *path,
						   uint64_t glb,
						   voe_base_arena *scratch)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	voe_3d_model_cooked_key key = { .glb = glb, .parts = 0 };

	// No `.parts`, or one that went, leaves its stamp 0.
	if (!voe_platform_file_stamp(
		    voe_game_models_parts_file(folder, path, scratch), &key.parts))
		key.parts = 0;
	voe_base_arena_rewind(scratch, mark);
	return key;
}

const voe_3d_model_prepared *
voe_game_models_cooked_take(const char *folder, const char *path,
			    voe_3d_model_cooked_key key, bool compare,
			    voe_base_arena *arena)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
	const char *file = voe_game_models_cooked_file(folder, path, arena);
	const voe_3d_model_prepared *prepared = NULL;
	voe_3d_model_cooked_key kept;
	const uint8_t *bytes = NULL;
	size_t size = 0;

	if (voe_platform_file_exists(file))
		bytes = voe_platform_file_read(file, arena, &size, NULL);
	if (bytes != NULL)
		prepared = voe_3d_model_cooked_read(arena, bytes, size, &kept);
	if (prepared != NULL &&
	    (!compare || (kept.glb == key.glb && kept.parts == key.parts)))
		return prepared;
	voe_base_arena_rewind(arena, mark);
	return NULL;
}

void voe_game_models_cooked_save(const char *folder, const char *path,
				 const voe_3d_model_prepared *prepared,
				 const voe_assets_model *model,
				 voe_3d_model_cooked_key key,
				 voe_render_device *device,
				 voe_base_arena *arena)
{
	voe_assets_image *pictures = voe_base_arena_push(
		arena, (model->image_count + 1) * sizeof(*pictures));
	voe_assets_bytes copy;
	const char *cache;
	voe_base_error unsaved = VOE_BASE_OK;

	VOE_BASE_ASSERT(pictures != NULL, "no room for the pictures");
	for (uint32_t i = 0; i < model->image_count; i++) {
		pictures[i] = voe_game_models_compress(
			device, &model->images[i],
			voe_3d_model_cooked_kind(model, i), arena);
		// Render said why; 3d lays out blocks alone.
		if (pictures[i].format == VOE_ASSETS_IMAGE_RGBA8)
			return;
	}
	copy = voe_3d_model_cooked_write(arena, prepared, pictures, key);
	cache = voe_platform_path_join(arena, folder, "Cache/models");
	VOE_BASE_ASSERT(copy.bytes != NULL && copy.count > 0, "an empty copy");
	if (!voe_game_models_folders_make(folder, cache, path, arena, &unsaved) ||
	    !voe_platform_file_write(
		    voe_game_models_cooked_file(folder, path, arena), copy.bytes,
		    copy.count, &unsaved))
		VOE_BASE_ERROR("game",
			       "the cooked copy could not be saved in the "
			       "project's `Cache/models/`: %s",
			       voe_base_error_string(unsaved));
}

// What stale and cook walk: `paths`, then `table`'s maps.
struct inputs {
	const char *folder;
	const char *const *paths;
	uint32_t path_count;
	const voe_game_materials *table;
};

// How many inputs `in` has: its paths and four maps a row.
static uint32_t inputs_count(const struct inputs *in)
{
	VOE_BASE_ASSERT(in != NULL && in->table != NULL, "no inputs or table");
	VOE_BASE_ASSERT(in->paths != NULL || in->path_count == 0,
			"paths counted with no array");
	return in->path_count + in->table->count * MAPS;
}

// The map `map` of the table's maps, four a row: its path, "" for none.
static const char *map_path(const voe_game_materials *table, uint32_t map)
{
	const voe_assets_material_file *values =
		&table->materials[map / MAPS].values;
	const char *const paths[MAPS] = { values->colormap, values->normalmap,
					  values->ormmap, values->emissionmap };

	VOE_BASE_ASSERT(map < table->count * MAPS, "a map past the table's");
	VOE_BASE_ASSERT(paths[map % MAPS] != NULL, "a map with no path");
	return paths[map % MAPS];
}

// Whether a map before `map` has its path and kind: its copy counted there.
static bool map_named_before(const voe_game_materials *table, uint32_t map)
{
	const char *path = map_path(table, map);
	bool named = false;

	VOE_BASE_ASSERT(path[0] != '\0', "asking after no map");
	for (uint32_t m = 0; !named && m < map; m++)
		named = MAP_KINDS[m % MAPS] == MAP_KINDS[map % MAPS] &&
			strcmp(map_path(table, m), path) == 0;
	return named;
}

// Whether the `.glb` `path` has a stamp and its copy's head is missing, will
// not read or holds another key. Scratch rewound.
static bool model_stale(const char *folder, const char *path,
			voe_base_arena *scratch)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	const char *file = voe_game_models_cooked_file(folder, path, scratch);
	uint8_t head[VOE_3D_MODEL_COOKED_HEAD];
	voe_3d_model_cooked_key key;
	voe_3d_model_cooked_key kept = { 0 };
	uint64_t stamp = 0;
	size_t count = 0;
	bool stale = false;

	VOE_BASE_ASSERT(file != NULL, "a copy with no path");
	if (voe_platform_file_stamp(voe_platform_path_join(scratch, folder,
							   path),
				    &stamp)) {
		key = voe_game_models_cooked_key(folder, path, stamp, scratch);
		// Asked first, so a missing copy says nothing on stderr.
		stale = !voe_platform_file_exists(file) ||
			!voe_platform_file_read_head(file, head, sizeof(head),
						     &count, NULL) ||
			!voe_3d_model_cooked_key_of(head, count, &kept) ||
			kept.glb != key.glb || kept.parts != key.parts;
	}
	voe_base_arena_rewind(scratch, mark);
	return stale;
}

// Whether the map `path` of `kind` has a stamp, kept in `*stamp` (0 for
// none), and its copy's head is missing, will not read or holds another key.
// Scratch rewound.
static bool map_stale(const char *folder, const char *path,
		      voe_assets_image_kind kind, uint64_t *stamp,
		      voe_base_arena *scratch)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	const char *file = voe_game_models_picture_file(folder, path, kind,
							scratch);
	uint8_t head[VOE_ASSETS_IMAGE_COOKED_HEAD];
	uint64_t key = 0;
	size_t count = 0;
	bool stale = false;

	VOE_BASE_ASSERT(stamp != NULL && file != NULL, "no stamp or copy");
	if (!voe_platform_file_stamp(voe_platform_path_join(scratch, folder,
							    path),
				     stamp))
		*stamp = 0;
	else
		stale = !voe_platform_file_exists(file) ||
			!voe_platform_file_read_head(file, head, sizeof(head),
						     &count, NULL) ||
			!voe_assets_image_cooked_key(head, count, &key) ||
			key != *stamp;
	voe_base_arena_rewind(scratch, mark);
	return stale;
}

// Whether input `index` of `in` is stale, a map's stamp kept in `*stamp`: a
// path that is no `.glb`, an empty map or one named before is not.
static bool stale_at(const struct inputs *in, uint32_t index, uint64_t *stamp,
		     voe_base_arena *scratch)
{
	uint32_t map = index - in->path_count;

	VOE_BASE_ASSERT(index < inputs_count(in), "an input past the inputs");
	VOE_BASE_ASSERT(stamp != NULL, "no stamp");
	*stamp = 0;
	if (index < in->path_count)
		return voe_game_models_is_glb(in->paths[index]) &&
		       model_stale(in->folder, in->paths[index], scratch);
	return map_path(in->table, map)[0] != '\0' &&
	       !map_named_before(in->table, map) &&
	       map_stale(in->folder, map_path(in->table, map),
			 MAP_KINDS[map % MAPS], stamp, scratch);
}

// The map `path` read, decoded as a picture of `kind` and saved as its copy
// keyed `stamp`, under its path as the subject; one that will not read or
// decode is said at its cause and skipped. Scratch rewound.
static void map_cook(const char *folder, const char *path,
		     voe_assets_image_kind kind, uint64_t stamp,
		     voe_render_device *device, voe_base_arena *scratch)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	voe_base_error error = VOE_BASE_OK;
	voe_assets_image image = { 0 };
	const uint8_t *bytes;
	size_t size = 0;

	VOE_BASE_ASSERT(path != NULL && path[0] != '\0', "cooking no map");
	voe_base_report_subject_push(path);
	bytes = voe_platform_file_read(voe_platform_path_join(scratch, folder,
							      path),
				       scratch, &size, &error);
	if (bytes != NULL && !voe_3d_model_picture_is(path))
		VOE_BASE_ERROR("game",
			       "the map `%s` is not a PNG or JPEG picture; save it as PNG",
			       path);
	else if (bytes != NULL &&
		 voe_3d_model_picture_decode(path, bytes, size, scratch, &image,
					     &error))
		(void)voe_game_models_picture_save(folder, path, &image, kind,
						   stamp, device, scratch);
	voe_base_report_subject_pop();
	voe_base_arena_rewind(scratch, mark);
}

unsigned voe_game_models_stale(const char *folder, const char *const *paths,
			       uint32_t path_count,
			       const voe_game_materials *table,
			       voe_base_arena *scratch)
{
	struct inputs in = { folder, paths, path_count, table };
	unsigned total = 0;
	uint64_t stamp = 0;

	VOE_BASE_ASSERT(folder != NULL && scratch != NULL,
			"no folder or scratch");
	for (uint32_t i = 0; i < inputs_count(&in); i++)
		total += stale_at(&in, i, &stamp, scratch);
	VOE_BASE_ASSERT(total <= inputs_count(&in), "more stale than inputs");
	return total;
}

// The stale `.glb`s from `*next` on gathered into `batch`, up to its `room`;
// one named earlier in the batch ends it, so the two never cook at once.
static void models_gather(const struct inputs *in, uint32_t *next,
			  uint32_t room, voe_game_models_batch *batch,
			  voe_base_arena *scratch)
{
	uint32_t first = *next;
	bool twice = false;
	uint64_t stamp = 0;

	VOE_BASE_ASSERT(in != NULL && next != NULL && batch != NULL,
			"no inputs, next or batch");
	for (; !twice && *next < in->path_count &&
	       voe_game_models_batch_count(batch) < room;
	     (*next)++) {
		for (uint32_t j = first; !twice && j < *next; j++)
			twice = strcmp(in->paths[j], in->paths[*next]) == 0;
		if (twice)
			break;
		if (stale_at(in, *next, &stamp, scratch))
			voe_game_models_batch_add(batch, in->paths[*next], 0,
						  true);
	}
	VOE_BASE_ASSERT(*next <= in->path_count, "gathered past the paths");
}

bool voe_game_models_cook(const char *folder, const char *const *paths,
			  uint32_t path_count, const voe_game_materials *table,
			  voe_render_device *device, voe_base_arena *scratch,
			  voe_game_progress *progress)
{
	struct inputs in = { folder, paths, path_count, table };
	unsigned total = voe_game_models_stale(folder, paths, path_count, table,
					       scratch);
	uint32_t workers = voe_platform_burst_workers(true);
	uint32_t room = workers < 4 ? workers : 4;
	uint32_t next = 0;
	unsigned done = 0;

	VOE_BASE_ASSERT(folder != NULL && scratch != NULL && device != NULL,
			"no folder, scratch or device");
	while (next < path_count) {
		voe_game_models_batch *batch = voe_game_models_batch_new(
			folder, room, VOE_GAME_MODELS_COOK_STALE, device,
			progress);
		voe_game_models_times times = { 0 };
		uint32_t count;

		models_gather(&in, &next, room, batch, scratch);
		count = voe_game_models_batch_count(batch);
		if (count > 0 && voe_game_progress_stopped(progress)) {
			voe_game_models_batch_destroy(batch);
			return false;
		}
		voe_game_progress_set(progress, "Preparing models", done, total);
		// A `.glb` that will not read or parse is said at its cause.
		voe_game_models_batch_prepare(batch, &times);
		voe_game_models_batch_destroy(batch);
		done += count;
	}
	for (uint32_t i = path_count; i < inputs_count(&in); i++) {
		uint64_t stamp = 0;

		if (!stale_at(&in, i, &stamp, scratch))
			continue;
		if (voe_game_progress_stopped(progress))
			return false;
		voe_game_progress_set(progress, "Preparing models", done, total);
		map_cook(folder, map_path(table, i - path_count),
			 MAP_KINDS[(i - path_count) % MAPS], stamp, device,
			 scratch);
		done++;
	}
	voe_game_progress_set(progress, "Preparing models", done, total);
	VOE_BASE_ASSERT(done <= inputs_count(&in), "more cooked than inputs");
	return true;
}
