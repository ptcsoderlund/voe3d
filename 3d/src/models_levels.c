// A loaded `.glb`'s levels put on the card beside its parts (3d/models.h): the
// own levels model_prepare.c baked, each part joined to level 0's by material,
// or the made ones of the levels file it read and checked (3d/model_levels.h),
// each part its file's indices alone; and their free. Nothing here bakes or
// reads. models.c calls these after a put's parts and wherever it frees parts.
//
// LEVEL 0 IS THE PARTS': _mirror copies their geometries into levels[0], never
// freed here. Levels 1 on are this file's.
//
// MADE LEVELS SHARE LEVEL 0'S VERTICES (0444): each part is
// voe_render_geometry_create_sharing over its level-0 part, so it costs its
// indices alone, and its free must come before level 0's geometries are
// destroyed, `render` asserting on a base destroyed under a sharer. Own levels
// keep vertices of their own.
//
// A LEVEL IS COUNTED BEFORE ITS FIRST UPLOAD, so a device refusing room part
// way leaves `level_count` covering what was made, and _free finds it; an id
// never made stays zeroed, generation 0 never being handed out.
//
// THE BOUNDS COME FROM voe_3d_model_levels_bounds (3d/model_levels.h), capped
// and spread there: measured by the file's errors and swaps for a made level
// always, for an own level when the prepare kept the file's measures, 2^-k
// otherwise. The radius is the entry's, set by the put before the levels.
#include "models_store.h"

#include <3d/model_levels.h>
#include <3d/models.h>
#include <base/assert.h>

void voe_3d_models_levels_mirror(voe_3d_model_entry *entry)
{
	VOE_BASE_ASSERT(entry != NULL, "mirroring no entry");
	VOE_BASE_ASSERT(entry->part_count <= VOE_3D_MODEL_PARTS,
			"an entry past its parts");

	for (uint32_t p = 0; p < entry->part_count; p++)
		entry->levels[0].geometries[p] = entry->parts[p].geometry;
}

void voe_3d_models_levels_free(voe_render_device *device,
			       const voe_3d_model_entry *entry)
{
	VOE_BASE_ASSERT(entry != NULL, "freeing the levels of no entry");
	VOE_BASE_ASSERT(entry->level_count <= VOE_3D_MODEL_LEVELS,
			"an entry past its levels");

	for (uint32_t k = 1; k < entry->level_count; k++) {
		for (uint32_t p = 0; p < VOE_3D_MODEL_PARTS; p++) {
			if (entry->levels[k].geometries[p].generation != 0)
				(void)voe_render_geometry_destroy(
					device, entry->levels[k].geometries[p]);
		}
	}
}

// Own level k's parts uploaded, each as the level-0 part of its material; a
// part matching none is left out.
static bool upload_own_level(voe_render_device *device,
			     const voe_3d_model_prepared *prepared, uint32_t k,
			     voe_3d_model_entry *entry, voe_base_error *error)
{
	const voe_3d_model_bake *bake = &prepared->own_levels[k];

	for (uint32_t q = 0; q < bake->part_count; q++) {
		const voe_3d_model_bake_part *part = &bake->parts[q];
		const uint32_t p =
			voe_3d_models_part_of(&prepared->bake, part->material);

		if (p == VOE_3D_MODEL_PARTS)
			continue;
		if (!voe_render_geometry_create(
			    device, bake->vertices + part->first_vertex,
			    part->vertex_count, bake->indices + part->first_index,
			    part->index_count, &entry->levels[k].geometries[p],
			    error))
			return false;
	}
	return true;
}

// The own levels, bounded by the file's errors and swaps when the prepare
// kept them, or by 2^-k.
static bool upload_own(voe_render_device *device,
		       const voe_3d_model_prepared *prepared,
		       voe_3d_model_entry *entry, voe_base_error *error)
{
	float bounds[VOE_3D_MODEL_LEVELS];

	entry->levels_own = true;
	voe_3d_model_levels_bounds(entry->radius, prepared->read.errors,
				   prepared->read.swaps, prepared->own_count,
				   prepared->measured, bounds);
	for (uint32_t k = 1; k < prepared->own_count; k++) {
		entry->level_count = k + 1;
		entry->levels[k].bound = bounds[k];
		if (!upload_own_level(device, prepared, k, entry, error))
			return false;
	}
	return true;
}

// Each made level's parts as indices alone over level 0's part geometries,
// the ones _mirror copies into levels[0] once the load is kept (part_count is
// not set before); the prepare has kept every index below its part's
// vertices.
static bool upload_made(voe_render_device *device,
			const voe_3d_model_levels *read,
			voe_3d_model_entry *entry, voe_base_error *error)
{
	float bounds[VOE_3D_MODEL_LEVELS];

	voe_3d_model_levels_bounds(entry->radius, read->errors, read->swaps,
				   read->level_count, true, bounds);
	for (uint32_t k = 1; k < read->level_count; k++) {
		entry->level_count = k + 1;
		entry->levels[k].bound = bounds[k];
		for (uint32_t p = 0; p < read->part_count; p++) {
			if (read->index_counts[k][p] > 0 &&
			    !voe_render_geometry_create_sharing(
				    device, entry->parts[p].geometry,
				    read->indices[k][p],
				    read->index_counts[k][p],
				    &entry->levels[k].geometries[p], error))
				return false;
		}
	}
	return true;
}

bool voe_3d_models_levels_upload(voe_render_device *device,
				 const voe_3d_model_prepared *prepared,
				 voe_3d_model_entry *entry,
				 voe_base_error *error)
{
	VOE_BASE_ASSERT(device != NULL, "uploading levels with no device");
	VOE_BASE_ASSERT(prepared != NULL && entry != NULL,
			"uploading the levels of nothing");
	VOE_BASE_ASSERT(entry->level_count == 1, "levels uploaded twice");

	if (prepared->own)
		return upload_own(device, prepared, entry, error);
	if (!prepared->made)
		return true;
	return upload_made(device, &prepared->read, entry, error);
}
