// The cooked copies of game/src/models_cooked.h: a model's and a map's copy,
// each its path, take and save, a model's key, and the folder and `.parts`
// helpers the levels files share. A take asks whether the copy is there before reading it, so a missing
// copy says nothing on stderr; a folder already there is found by a listing
// for the same reason. Every helper pushes its paths on the arena it is given.
#include "models_cooked.h"

#include <base/assert.h>
#include <base/report.h>

#include <platform/file.h>
#include <platform/folder.h>
#include <platform/path.h>

#include <ctype.h>
#include <string.h>

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

// Makes the folder `path` unless a listing finds one there. False, `error`
// set, when it could not be made.
static bool make_absent(const char *path, voe_base_arena *scratch,
			voe_base_error *error)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	voe_platform_folder_listing listing;
	bool there = voe_platform_folder_list(path, scratch, &listing, NULL);

	VOE_BASE_ASSERT(path != NULL, "making no folder");
	voe_base_arena_rewind(scratch, mark);
	return there || voe_platform_folder_create(path, error);
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

voe_assets_image voe_game_models_picture_save(const char *folder,
					      const char *path,
					      const voe_assets_image *image,
					      voe_assets_image_kind kind,
					      uint64_t stamp,
					      voe_base_arena *arena)
{
	voe_assets_image chain = voe_assets_image_mips(arena, image, kind);
	voe_assets_bytes copy =
		voe_assets_image_cooked_write(arena, &chain, kind, stamp);
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
	return chain;
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
				 voe_3d_model_cooked_key key,
				 voe_base_arena *arena)
{
	voe_assets_bytes copy = voe_3d_model_cooked_write(arena, prepared, key);
	const char *cache = voe_platform_path_join(arena, folder, "Cache/models");
	voe_base_error unsaved = VOE_BASE_OK;

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
