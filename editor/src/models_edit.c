// The editor's model store's landscape edits models.h declares: heights
// stamped by a brush and put back, paint put back, layers set, foliage
// scattered, erased and put back, kinds, their looks and the layers' growth
// set, each handed to 3d/models.h on the store's own entry, and entries
// renamed. Also a landscape's size and cells found, from the loaded entry or
// its file, and the reshape: resampled, written at once and named in
// `resized` for models.c's next update to load again. An edit is nothing for
// a path that is no loaded landscape, as 3d/models.h has it. The store's
// struct is models_store.h's, shared with models.c.
#include "models_store.h"

#include <base/assert.h>
#include <base/error.h>

#include <platform/file.h>
#include <platform/path.h>

#include <stdlib.h>
#include <string.h>

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

void voe_editor_models_look_set(voe_editor_models *models, const char *path,
				uint32_t kind, voe_assets_landscape_look look)
{
	VOE_BASE_ASSERT(models != NULL && models->store != NULL,
			"setting a kind's look in no store");
	VOE_BASE_ASSERT(path != NULL, "setting a kind's look on no path");
	voe_3d_models_landscape_look_set(models->store, path, kind, look);
}

void voe_editor_models_grow_set(voe_editor_models *models, const char *path,
				uint32_t layer, uint32_t kind, uint16_t density)
{
	VOE_BASE_ASSERT(models != NULL && models->store != NULL,
			"setting a layer's growth in no store");
	VOE_BASE_ASSERT(path != NULL && density <= VOE_ASSETS_LANDSCAPE_GROW_MAX,
			"setting a growth on no path or past its most");
	voe_3d_models_landscape_grow_set(models->store, path, layer, kind,
					 density);
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
				       voe_editor_disk_watch *watch,
				       voe_editor_notice *why)
{
	const voe_3d_model_entry *entry;
	struct voe_base_arena_mark mark;
	const char *full;
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
	full = voe_platform_path_join(scratch, folder, path);
	written = voe_platform_file_write(full, (const uint8_t *)text.text,
					  text.size, &error);
	if (!written) {
		voe_editor_notice_set(why, "Could not save %s: %s", path,
				      voe_base_error_string(error));
		goto rewind;
	}
	if (watch != NULL)
		voe_editor_disk_watch_wrote(watch, full);
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
