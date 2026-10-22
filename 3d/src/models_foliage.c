// The model store's foliage calls (3d/models_foliage.h): each finds a loaded
// landscape's entry, hands its grid and its held placed transforms to
// 3d/foliage.h, and marks the entry edited when a copy or a kind changed. A
// scatter or erase that changed a copy, every put of copies, every kind set
// and every changed kind size raise the store's `changes`, so the bounce
// relights (0451 point 3); a kind size places that kind's copies again.
//
// Used by the editor: the scatter and erase brushes while a stroke is held,
// the put for its undo step, the kind set for a dropped model; and by what
// draws the copies, through _placed.
//
// Constraints: a path is found by models_landscape.c's walk over the entries;
// the placed transforms are models_landscape.c's, made at the load.
#include <3d/models_foliage.h>

#include "models_store.h"

#include <3d/foliage.h>
#include <assets/landscape.h>
#include <base/assert.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

uint32_t voe_3d_models_landscape_scatter(voe_3d_models *models,
					 const char *path,
					 const voe_3d_foliage_brush *brush,
					 float x, float z, float seconds,
					 uint64_t *seed,
					 voe_assets_landscape_copy *added)
{
	VOE_BASE_ASSERT(models != NULL && path != NULL,
			"scattering needs a store and a path");
	VOE_BASE_ASSERT(brush != NULL && seed != NULL && added != NULL,
			"scattering needs a brush, a seed and room");
	const uint32_t index = voe_3d_models_landscape_index(models, path);
	uint32_t count;

	if (index == VOE_3D_MODELS)
		return 0;
	count = voe_3d_foliage_scatter(models->entries[index].landscape,
				       models->held[index].placed, brush, x, z,
				       seconds, seed, added);
	if (count > 0) {
		models->entries[index].edited = true;
		models->changes++;
	}
	return count;
}

uint32_t voe_3d_models_landscape_erase(voe_3d_models *models, const char *path,
				       const voe_3d_foliage_brush *brush,
				       float x, float z,
				       voe_assets_landscape_copy *removed,
				       uint32_t room)
{
	VOE_BASE_ASSERT(models != NULL && path != NULL,
			"erasing needs a store and a path");
	VOE_BASE_ASSERT(brush != NULL && (removed != NULL || room == 0),
			"erasing needs a brush and room");
	const uint32_t index = voe_3d_models_landscape_index(models, path);
	uint32_t count;

	if (index == VOE_3D_MODELS)
		return 0;
	count = voe_3d_foliage_erase(models->entries[index].landscape,
				     models->held[index].placed, brush, x, z,
				     removed, room);
	if (count > 0) {
		models->entries[index].edited = true;
		models->changes++;
	}
	return count;
}

// Edited when the count moved: an add adds and a removal removes, or neither.
// The store's changes rise either way.
void voe_3d_models_landscape_put_copies(voe_3d_models *models,
					const char *path,
					const voe_assets_landscape_copy *copies,
					uint32_t count, bool add)
{
	VOE_BASE_ASSERT(models != NULL && path != NULL,
			"putting copies needs a store and a path");
	VOE_BASE_ASSERT(copies != NULL || count == 0, "the copies put");
	const uint32_t index = voe_3d_models_landscape_index(models, path);

	if (index == VOE_3D_MODELS)
		return;
	voe_assets_landscape *land = models->entries[index].landscape;
	const uint32_t before = land->copy_count;

	voe_3d_foliage_put(land, models->held[index].placed, copies, count,
			   add);
	models->changes++;
	if (land->copy_count != before)
		models->entries[index].edited = true;
}

void voe_3d_models_landscape_kind_set(voe_3d_models *models, const char *path,
				      uint32_t index, const char *model)
{
	VOE_BASE_ASSERT(models != NULL && path != NULL,
			"setting a kind needs a store and a path");
	VOE_BASE_ASSERT(model != NULL &&
				strlen(model) < VOE_ASSETS_LANDSCAPE_KIND_PATH,
			"a kind's model path that fits");
	const uint32_t at = voe_3d_models_landscape_index(models, path);

	if (at == VOE_3D_MODELS)
		return;
	voe_assets_landscape *land = models->entries[at].landscape;

	VOE_BASE_ASSERT(index <= land->kind_count &&
				index < VOE_ASSETS_LANDSCAPE_KINDS,
			"a held kind or the next of sixteen");
	memcpy(land->kinds[index], model, strlen(model) + 1);
	if (index == land->kind_count) {
		land->kind_sizes[index] = 0;
		land->kind_count++;
	}
	models->entries[at].edited = true;
	models->changes++;
}

// One walk of the copies, each of kind `index` placed again at the new size.
void voe_3d_models_landscape_kind_size_set(voe_3d_models *models,
					   const char *path, uint32_t index,
					   uint16_t size)
{
	VOE_BASE_ASSERT(models != NULL && path != NULL,
			"setting a kind's size needs a store and a path");
	VOE_BASE_ASSERT(size == 0 ||
				(size >= VOE_ASSETS_LANDSCAPE_KIND_SIZE_MIN &&
				 size <= VOE_ASSETS_LANDSCAPE_KIND_SIZE_MAX),
			"a kind size the file can hold");
	const uint32_t at = voe_3d_models_landscape_index(models, path);

	if (at == VOE_3D_MODELS)
		return;
	voe_assets_landscape *land = models->entries[at].landscape;
	voe_scene_transform *placed = models->held[at].placed;

	VOE_BASE_ASSERT(index < land->kind_count, "a held kind");
	if (land->kind_sizes[index] == size)
		return;
	land->kind_sizes[index] = size;
	for (uint32_t i = 0; i < land->copy_count; i++)
		if (land->copies[i].kind == index)
			placed[i] = voe_3d_foliage_place(land, land->copies[i]);
	models->entries[at].edited = true;
	models->changes++;
}

const voe_scene_transform *
voe_3d_models_landscape_placed(const voe_3d_models *models,
			       const voe_3d_model_entry *entry)
{
	VOE_BASE_ASSERT(models != NULL, "placed copies need a store");
	VOE_BASE_ASSERT(entry != NULL, "placed copies need an entry");

	for (uint32_t i = 0; i < models->count; i++) {
		if (&models->entries[i] != entry)
			continue;
		if (!entry->loaded || entry->landscape == NULL)
			return NULL;
		return models->held[i].placed;
	}
	return NULL;
}
