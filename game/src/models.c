// The model loader of game/include/game/models.h: a model row's path or an
// emitter's texture joined onto the folder, stamped, read into scratch and
// handed to the store, the soft dot loaded when any emitter is there and the
// water record when any water is, each
// failure one line on stderr and counted. Scratch is rewound to where it stood after each
// file, so a caller's own scratch data survives the call. With a progress, the
// distinct unread paths are counted first and the stop flag asked before each.
// A cooked landscape's millimetres become metres in scratch, rewound after it,
// its layer paths copied and its paint widened to both planes there too, and
// the watch passes over every landscape and material entry. A `.glb` loads
// with its levels file, `<folder>/Cache/levels/<path>.levels`, read on scratch
// or made there and written, its folders made first when absent; made with
// the `[Levels]` of the `.parts` beside the `.glb`, the defaults when there is
// none or it will not parse. A fresh file kept whose swaps differ from those of
// a `.parts` that reads has them patched into a copy on scratch, written back
// and handed on, not made again; with no `.parts` its own swaps stand. One held
// path is loaded again on asking, its stamp not asked. The triangle counts of
// a `.glb`'s levels are read for the LODs panel, nothing made or written.
#include <game/models.h>

#include <3d/emitter_component.h>
#include <3d/model_component.h>
#include <3d/model_levels.h>
#include <3d/water_component.h>

#include <assets/parts.h>

#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>

#include <platform/file.h>
#include <platform/folder.h>
#include <platform/path.h>

#include <ctype.h>
#include <stddef.h>
#include <string.h>

// Counts one failure of `path`, the store's copy kept when it is the first.
static void count_failure(voe_game_models_failures *failures,
			  const voe_3d_models *models, const char *path,
			  voe_base_error error)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(models, path);

	VOE_BASE_ASSERT(failures != NULL, "counting into no failures");
	VOE_BASE_ASSERT(path != NULL, "counting a failure with no path");
	VOE_BASE_ERROR("game", "could not read the model %s: %s", path,
		       voe_base_error_string(error));
	// The dot keeps no failed entry; its "" is a literal, valid for ever.
	if (failures->count == 0 && entry != NULL)
		failures->first = entry->path;
	else if (failures->count == 0 && path[0] == '\0')
		failures->first = "";
	failures->count++;
}

// Whether `path` ends `.glb`, compared without case.
static bool is_glb(const char *path)
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

// Makes the folder `path` unless a listing finds one there, so a folder
// already there says nothing on stderr. False when it could not be made.
static bool make_absent(const char *path, voe_base_arena *scratch)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	voe_platform_folder_listing listing;
	bool there = voe_platform_folder_list(path, scratch, &listing, NULL);

	VOE_BASE_ASSERT(path != NULL, "making no folder");
	voe_base_arena_rewind(scratch, mark);
	return there || voe_platform_folder_create(path, NULL);
}

// Makes `levels` (`folder`'s Cache/levels) and each folder of the
// `/`-separated `path` under it, every one only when absent. False when one
// could not be made.
static bool make_levels_folders(const char *folder, const char *levels,
				const char *path, voe_base_arena *scratch)
{
	size_t length = strlen(path);
	char *prefix = voe_base_arena_push(scratch, length + 1);
	bool made = make_absent(voe_platform_path_join(scratch, folder, "Cache"),
				scratch) &&
		    make_absent(levels, scratch);

	VOE_BASE_ASSERT(prefix != NULL, "no room for a path");
	memcpy(prefix, path, length + 1);
	for (size_t i = 0; made && i < length; i++) {
		if (path[i] != '/')
			continue;
		prefix[i] = '\0';
		made = make_absent(voe_platform_path_join(scratch, levels, prefix),
				   scratch);
		prefix[i] = '/';
	}
	return made;
}

// The `[Levels]` of the `.parts` beside `folder`'s `.glb` `path` (0430 point
// 5), `*parted` whether that file was there and read: the defaults, silently,
// when there is no such file, and when it will not parse, the parts reader
// having said why. Scratch rewound.
static voe_assets_levels levels_asked(const char *folder, const char *path,
				      voe_base_arena *scratch, bool *parted)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	// A prepare of a path not ending `.glb` looks beside it whole.
	size_t stem = strlen(path) - (is_glb(path) ? sizeof(".glb") - 1 : 0);
	char *name = voe_base_arena_push(scratch, stem + sizeof(".parts"));
	voe_assets_levels levels = VOE_ASSETS_LEVELS_DEFAULT;
	voe_assets_parts parts;
	const char *file;
	const uint8_t *text = NULL;
	size_t size = 0;

	VOE_BASE_ASSERT(folder != NULL, "asking levels in no folder");
	VOE_BASE_ASSERT(name != NULL && parted != NULL, "no room or no parted");
	memcpy(name, path, stem);
	memcpy(name + stem, ".parts", sizeof(".parts"));
	file = voe_platform_path_join(scratch, folder, name);
	if (voe_platform_file_exists(file))
		text = voe_platform_file_read(file, scratch, &size, NULL);
	*parted = text != NULL &&
		  voe_assets_parts_read((const char *)text, size, &parts, NULL);
	if (*parted)
		levels = parts.levels;
	voe_base_arena_rewind(scratch, mark);
	return levels;
}

// `levels`, `folder`'s Cache/levels, joined with `path` and `.levels`.
static const char *levels_file(const char *levels, const char *path,
			       voe_base_arena *scratch)
{
	size_t length = strlen(path);
	char *name = voe_base_arena_push(scratch, length + sizeof(".levels"));

	VOE_BASE_ASSERT(name != NULL, "no room for a path");
	memcpy(name, path, length);
	memcpy(name + length, ".levels", sizeof(".levels"));
	return voe_platform_path_join(scratch, levels, name);
}

// The fresh levels file `bytes`, whose swaps `read` holds, handed on as it is
// unless `asked`'s swaps 1 to 5 differ (0436 point 7): then a copy on scratch
// with them set, written back to `file`, a failed write a line on stderr.
static const uint8_t *swaps_patched(const char *file, const char *path,
				    const uint8_t *bytes, size_t size,
				    const voe_3d_model_levels *read,
				    const voe_assets_levels *asked,
				    voe_base_arena *scratch)
{
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
	if (!voe_platform_file_write(file, copy, size, NULL))
		VOE_BASE_ERROR("game", "could not write the levels of %s", path);
	return copy;
}

// The levels file of `folder`'s `path`, whose bytes are `glb`, on scratch with
// its count in `*size`: the one in the Cache when it fits `glb` and `remake`
// is false, its swaps patched to a `.parts` that reads, else made and written,
// `*made` then true. A failed write is a line on stderr, and the levels just
// made or patched are handed back still. NULL, `error` set, only when `glb`
// will not parse.
static const uint8_t *levels_for(const char *folder, const char *path,
				 const uint8_t *glb, size_t glb_size,
				 bool remake, voe_base_arena *scratch,
				 size_t *size, bool *made, voe_base_error *error)
{
	const char *levels = voe_platform_path_join(scratch, folder,
						    "Cache/levels");
	const char *file = levels_file(levels, path, scratch);
	const uint8_t *bytes = NULL;
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
		return parted ? swaps_patched(file, path, bytes, *size, &read,
					      &asked, scratch) :
				bytes;
	bytes = voe_3d_model_levels_make(scratch, glb, glb_size, asked, size,
					 error);
	if (bytes == NULL)
		return NULL;
	*made = true;
	if (!make_levels_folders(folder, levels, path, scratch) ||
	    !voe_platform_file_write(file, bytes, *size, NULL))
		VOE_BASE_ERROR("game", "could not write the levels of %s", path);
	return bytes;
}

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
	glb = voe_platform_file_read(voe_platform_path_join(scratch, folder,
							    path),
				     scratch, &glb_size, NULL);
	file = levels_file(voe_platform_path_join(scratch, folder,
						  "Cache/levels"),
			   path, scratch);
	if (glb != NULL && voe_platform_file_exists(file))
		levels = voe_platform_file_read(file, scratch, &size, NULL);
	if (glb != NULL)
		count = voe_3d_model_levels_triangles(scratch, glb, glb_size,
						      levels, size, triangles,
						      own, fresh);
	voe_base_arena_rewind(scratch, mark);
	return count;
}

bool voe_game_models_prepare(const char *folder, const char *path,
			     voe_base_arena *scratch, bool remake, bool *made,
			     voe_base_error *error)
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
	glb = voe_platform_file_read(voe_platform_path_join(scratch, folder,
							    path),
				     scratch, &glb_size, error);
	ready = glb != NULL && levels_for(folder, path, glb, glb_size, remake,
					  scratch, &size, made, error) != NULL;
	voe_base_arena_rewind(scratch, mark);
	return ready;
}

// Loads `path` from `folder` if its stamp is not `known`, or whatever it is
// when `always`; a file with no stamp becomes a failed entry at 0, unless it
// already is one. A `.glb` loads with its levels file, made when missing or
// stale. False, with the category in `error`, when it failed.
static bool load_changed(voe_3d_models *models, voe_render_device *device,
			 const char *folder, const char *path, uint64_t known,
			 bool always, voe_base_arena *scratch,
			 voe_base_error *error)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	const char *full = voe_platform_path_join(scratch, folder, path);
	const uint8_t *bytes = NULL;
	const uint8_t *levels = NULL;
	voe_base_error unmade;
	uint64_t stamp = 0;
	size_t size = 0;
	size_t levels_size = 0;
	bool made;
	bool loaded = true;

	VOE_BASE_ASSERT(path[0] != '\0', "loading a model with no path");
	*error = VOE_BASE_OK;
	if (!voe_platform_file_stamp(full, &stamp)) {
		if (known != 0 || voe_3d_models_find(models, path) == NULL) {
			voe_3d_models_fail(models, path, 0);
			*error = VOE_BASE_ERROR_UNAVAILABLE;
			loaded = false;
		}
	} else if (always || stamp != known) {
		bytes = voe_platform_file_read(full, scratch, &size, error);
		if (bytes == NULL)
			voe_3d_models_fail(models, path, 0);
		// A `.glb` that will not parse fails the load the same way.
		if (bytes != NULL && is_glb(path))
			levels = levels_for(folder, path, bytes, size, false,
					    scratch, &levels_size, &made,
					    &unmade);
		loaded = bytes != NULL &&
			 voe_3d_models_load_levelled(models, device, path,
						     stamp, bytes, size, levels,
						     levels_size, error);
	}
	voe_base_arena_rewind(scratch, mark);
	VOE_BASE_ASSERT(loaded || *error != VOE_BASE_OK,
			"a failed model load with no category");
	return loaded;
}

bool voe_game_models_reload(voe_3d_models *models, voe_render_device *device,
			    const char *folder, const char *path,
			    voe_base_arena *scratch, voe_base_error *error)
{
	const voe_3d_model_entry *entry;

	VOE_BASE_ASSERT(models != NULL && device != NULL, "no store or device");
	VOE_BASE_ASSERT(folder != NULL && path != NULL && scratch != NULL &&
				error != NULL,
			"no folder, path, scratch or error");
	*error = VOE_BASE_OK;
	entry = voe_3d_models_find(models, path);
	if (entry == NULL)
		return true;
	return load_changed(models, device, folder, entry->path, entry->stamp,
			    true, scratch, error);
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

// How many distinct paths of `paths` files an update reads. Each path is
// compared with every earlier one, so a path named twice counts once; a
// lookup set of the paths would lift that square.
static unsigned count_unread(const voe_ecs_world *world,
			     const voe_3d_models *models, uint32_t paths)
{
	unsigned total = 0;

	for (uint32_t i = 0; i < paths; i++) {
		const char *path = path_at(world, i);
		bool earlier = false;

		for (uint32_t j = 0; !earlier && j < i; j++)
			earlier = strcmp(path_at(world, j), path) == 0;
		total += !earlier && unread(models, path);
	}
	return total;
}

voe_game_models_failures voe_game_models_update(const voe_ecs_world *world,
						voe_3d_models *models,
						voe_render_device *device,
						const char *folder,
						voe_base_arena *scratch,
						voe_game_progress *progress)
{
	uint32_t paths = voe_3d_model_count(world) + voe_3d_emitter_count(world);
	voe_game_models_failures failures = { 0 };
	unsigned done = 0;
	unsigned total = 0;
	voe_base_error error;

	VOE_BASE_ASSERT(models != NULL && device != NULL, "no store or device");
	VOE_BASE_ASSERT(folder != NULL && scratch != NULL,
			"no folder or scratch");
	if (progress != NULL)
		total = count_unread(world, models, paths);
	for (uint32_t i = 0; i < paths; i++) {
		const char *path = path_at(world, i);

		if (!unread(models, path))
			continue;
		if (voe_game_progress_stopped(progress))
			return failures;
		// A full store said so once; the update ends.
		if (voe_3d_models_count(models) == VOE_3D_MODELS)
			break;
		voe_game_progress_set(progress, "Loading models", done, total);
		if (!load_changed(models, device, folder, path, 0, false,
				  scratch, &error))
			count_failure(&failures, models, path, error);
		done++;
	}
	voe_game_progress_set(progress, "Loading models", done, total);
	if (voe_3d_emitter_count(world) > 0 &&
	    voe_3d_models_find(models, "") == NULL &&
	    !voe_3d_models_load_dot(models, device, &error))
		count_failure(&failures, models, "", error);
	if (voe_3d_water_count(world) > 0 &&
	    voe_3d_models_water(models) == NULL &&
	    !voe_3d_models_load_water(models, device, &error))
		count_failure(&failures, models, "", error);
	return failures;
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
		if (!voe_3d_models_load_landscape(models, device, cooked->path,
						  0, &landscape, &error))
			count_failure(&failures, models, cooked->path, error);
		voe_base_arena_rewind(scratch, mark);
	}
	return failures;
}

voe_game_models_failures voe_game_models_watch(voe_3d_models *models,
					       voe_render_device *device,
					       const char *folder,
					       voe_base_arena *scratch)
{
	voe_game_models_failures failures = { 0 };

	VOE_BASE_ASSERT(models != NULL && device != NULL, "no store or device");
	VOE_BASE_ASSERT(folder != NULL && scratch != NULL,
			"no folder or scratch");
	for (uint32_t i = 0; i < voe_3d_models_count(models); i++) {
		const voe_3d_model_entry *entry = voe_3d_models_at(models, i);
		voe_base_error error;

		// A landscape is never re-read by stamp (0379 point 2), nor a
		// material, which the editor reloads itself (0399 point 7).
		if (entry->landscape != NULL || entry->material)
			continue;
		if (!load_changed(models, device, folder, entry->path,
				  entry->stamp, false, scratch, &error))
			count_failure(&failures, models,
				      voe_3d_models_at(models, i)->path, error);
	}
	return failures;
}
