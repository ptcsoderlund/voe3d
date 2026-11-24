// The model store's grown trees (3d/models.h, 0515 point 3): a variation put
// wearing its species' bark and leaf, the species entry that holds its
// variations, and the variation one foliage copy draws by its kind's seed.
//
// A VARIATION IS A `.glb` PUT: models.c's _put at stamp 0, then each part's
// `wears` set from its glTF source, material 0 the bark and 1 the leaf, and
// the entry marked grown so _parts_set leaves it.
//
// A SPECIES IS AN ENTRY WITH NO PARTS, loaded, kept through models.c's keep
// like any other, so _find, _clear and the rename treat it as one. Its
// variations are indices into the table, which stay put: keep replaces a path
// where it stands and only _clear empties the table, the species with it.
// It holds nothing on a device, so keep is handed none.
//
// CONSTRAINTS: _species_set scans every entry once for `path#<v>`, which at
// VOE_3D_MODELS entries is nothing. A variation put after the set is not held
// until the set is called again.
#include "models_store.h"

#include <3d/models.h>
#include <3d/tree_grow.h>
#include <base/assert.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool voe_3d_models_put_grown(voe_3d_models *models, voe_render_device *device,
			     const char *path,
			     const voe_3d_model_prepared *prepared,
			     const char *bark, const char *leaf,
			     voe_base_error *error)
{
	voe_3d_model_entry *entry;

	VOE_BASE_ASSERT(bark != NULL && leaf != NULL, "a tree wearing no paths");
	VOE_BASE_ASSERT(strlen(bark) < VOE_ASSETS_MATERIAL_PATH &&
				strlen(leaf) < VOE_ASSETS_MATERIAL_PATH,
			"a tree's bark or leaf path past a material path's room");

	if (!voe_3d_models_put(models, device, path, 0, prepared, error))
		return false;
	entry = &models->entries[voe_3d_models_index(models, path)];
	for (uint32_t p = 0; p < entry->part_count; p++) {
		voe_3d_model_part *part = &entry->parts[p];

		snprintf(part->wears, sizeof(part->wears), "%s",
			 part->source == 0 ? bark :
			 part->source == 1 ? leaf :
					     "");
	}
	entry->grown = true;
	return true;
}

// The variation `path` names of the species `species`, `length` long: v when
// it is `species#<v>` with v below `count`, else `count`.
static uint32_t variation_of(const char *path, const char *species,
			     size_t length, uint32_t count)
{
	const char *digits = path + length + 1;
	char *end;
	unsigned long v;

	if (strncmp(path, species, length) != 0 || path[length] != '#' ||
	    !isdigit((unsigned char)digits[0]))
		return count;
	v = strtoul(digits, &end, 10);
	return *end == '\0' && v < count ? (uint32_t)v : count;
}

bool voe_3d_models_species_set(voe_3d_models *models, const char *path,
			       uint32_t count)
{
	const uint32_t index = voe_3d_models_index(models, path);
	const size_t length = strlen(path);
	voe_3d_model_entry entry;
	entry_held held;

	VOE_BASE_ASSERT(models != NULL && path != NULL && path[0] != '\0',
			"a species with no store or path");
	VOE_BASE_ASSERT(count > 0 && count <= VOE_3D_TREE_VARIATIONS,
			"a species of 1 to 16 variations");
	VOE_BASE_ASSERT(index == VOE_3D_MODELS ||
				(models->entries[index].part_count == 0 &&
				 models->entries[index].landscape == NULL),
			"a species over an entry with parts");

	voe_3d_models_entry_new(path, 0, &entry, &held);
	entry.loaded = true;
	entry.variation_count = count;
	for (uint32_t v = 0; v < count; v++)
		entry.variations[v] = VOE_3D_MODELS;
	for (uint32_t i = 0; i < models->count; i++) {
		const uint32_t v = variation_of(models->entries[i].path, path,
						length, count);

		if (v < count)
			entry.variations[v] = i;
	}
	return voe_3d_models_keep(models, NULL, &entry, &held);
}

const voe_3d_model_entry *
voe_3d_models_variation(const voe_3d_models *models,
			const voe_3d_model_entry *entry, uint32_t seed,
			int32_t x_mm, int32_t z_mm)
{
	uint32_t index;

	VOE_BASE_ASSERT(models != NULL && entry != NULL,
			"a variation of no store or entry");
	VOE_BASE_ASSERT(entry->variation_count <= VOE_3D_TREE_VARIATIONS,
			"a species past its variations");

	if (entry->variation_count == 0)
		return entry;
	index = entry->variations[voe_3d_tree_pick(seed, x_mm, z_mm,
						   entry->variation_count)];
	if (index >= models->count || !models->entries[index].loaded)
		return NULL;
	return &models->entries[index];
}
