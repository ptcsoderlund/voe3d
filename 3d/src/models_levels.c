// A loaded `.glb`'s levels put on the card beside its parts (3d/models.h): the
// file's own levels baked one at a time and joined to level 0's parts by
// material, or the made ones a levels file holds (3d/model_levels.h), each
// part its file's indices alone; and their free. models.c calls these after a
// load's parts and wherever it frees parts.
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
// A STALE OR WRONG LEVELS FILE LOADS LEVEL 0 ALONE (0426 point 1): a read
// saying false, a part count not the bake's, or any index at or past its part's
// vertices, all found before anything is uploaded.
//
// A LEVEL'S BOUND IS 0.002 × RADIUS ÷ ITS ERROR ÷ ITS SWAP IN THE FILE (0436
// points 4, 6), infinite for error nought, then made non-increasing: a made
// level's always, an own level's when the fresh file is own and counts the
// bake's own levels. Otherwise an own level's bound is 2^-k, its swap 1.
//
// AN OWN LEVEL MATCHING NO PART ENDS THE LIST: it would draw nothing, and the
// levels after it are farther still.
#include "models_store.h"

#include <3d/model_levels.h>
#include <3d/models.h>
#include <base/assert.h>

#include <math.h>

// A level's error kept under this share of half the screen (0425 point 3).
#define SCREEN_ERROR 0.002f

// Level k's bound from `read`'s error and swap, infinite for error nought.
static float error_bound(const voe_3d_model_entry *entry,
			 const voe_3d_model_levels *read, uint32_t k)
{
	const float made = read->errors[k];

	return made > 0.0f ? SCREEN_ERROR * entry->radius / made / read->swaps[k] :
			     INFINITY;
}

void voe_3d_models_sphere(const voe_3d_model_bake *bake,
			  voe_3d_model_entry *entry)
{
	voe_math_float3 low = { INFINITY, INFINITY, INFINITY };
	voe_math_float3 high = { -INFINITY, -INFINITY, -INFINITY };
	float most = 0.0f;

	VOE_BASE_ASSERT(bake != NULL, "the sphere of no bake");
	VOE_BASE_ASSERT(entry != NULL, "the sphere into no entry");

	if (bake->vertex_count == 0)
		return;
	for (uint32_t v = 0; v < bake->vertex_count; v++) {
		low = voe_math_float3_min(low, bake->vertices[v].position);
		high = voe_math_float3_max(high, bake->vertices[v].position);
	}
	entry->centre = voe_math_float3_scale(voe_math_float3_add(low, high),
					      0.5f);
	for (uint32_t v = 0; v < bake->vertex_count; v++)
		most = fmaxf(most, voe_math_float3_length(voe_math_float3_sub(
					   bake->vertices[v].position,
					   entry->centre)));
	entry->radius = most;
}

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

// The level-0 part baked from glTF material `material`; VOE_3D_MODEL_PARTS
// when none was.
static uint32_t part_of(const voe_3d_model_bake *bake, uint32_t material)
{
	for (uint32_t p = 0; p < bake->part_count; p++) {
		if (bake->parts[p].material == material)
			return p;
	}
	return VOE_3D_MODEL_PARTS;
}

// Own level k baked, each part uploaded as the level-0 part of its material,
// counting into `*matched` those that had one.
static bool upload_own_level(voe_render_device *device, voe_base_arena *scratch,
			     const voe_assets_model *model,
			     const voe_3d_model_bake *zero, uint32_t k,
			     voe_3d_model_entry *entry, uint32_t *matched,
			     voe_base_error *error)
{
	voe_3d_model_bake bake;

	if (!voe_3d_model_bake_create(scratch, model, k, &bake, error))
		return false;
	for (uint32_t q = 0; q < bake.part_count; q++) {
		const voe_3d_model_bake_part *part = &bake.parts[q];
		const uint32_t p = part_of(zero, part->material);

		if (p == VOE_3D_MODEL_PARTS)
			continue;
		if (!voe_render_geometry_create(
			    device, bake.vertices + part->first_vertex,
			    part->vertex_count, bake.indices + part->first_index,
			    part->index_count, &entry->levels[k].geometries[p],
			    error))
			return false;
		(*matched)++;
	}
	return true;
}

// The own levels, bounded by `measured`'s errors and swaps, or by 2^-k when it
// is NULL.
static bool upload_own(voe_render_device *device, voe_base_arena *scratch,
		       const voe_assets_model *model,
		       const voe_3d_model_bake *zero, uint32_t count,
		       const voe_3d_model_levels *measured,
		       voe_3d_model_entry *entry, voe_base_error *error)
{
	entry->levels_own = true;
	for (uint32_t k = 1; k < count && k < VOE_3D_MODEL_LEVELS; k++) {
		const float bound = measured != NULL ?
					    error_bound(entry, measured, k) :
					    ldexpf(1.0f, -(int)k);
		uint32_t matched = 0;

		entry->level_count = k + 1;
		entry->levels[k].bound =
			fminf(bound, entry->levels[k - 1].bound);
		if (!upload_own_level(device, scratch, model, zero, k, entry,
				      &matched, error))
			return false;
		if (matched == 0) {
			entry->level_count = k;
			break;
		}
	}
	return true;
}

// Whether `read` fits `bake`: made levels, its part count, and every index
// below its part's vertex count.
static bool fits(const voe_3d_model_levels *read, const voe_3d_model_bake *bake)
{
	if (read->own || read->part_count != bake->part_count)
		return false;
	for (uint32_t k = 1; k < read->level_count; k++) {
		for (uint32_t p = 0; p < read->part_count; p++) {
			for (uint32_t i = 0; i < read->index_counts[k][p]; i++) {
				if (read->indices[k][p][i] >=
				    bake->parts[p].vertex_count)
					return false;
			}
		}
	}
	return true;
}

// Each made level's parts as indices alone over level 0's part geometries,
// the ones _mirror copies into levels[0] once the load is kept (part_count is
// not set before); `fits` has kept every index below its part's vertices.
static bool upload_made(voe_render_device *device,
			const voe_3d_model_levels *read,
			voe_3d_model_entry *entry, voe_base_error *error)
{
	for (uint32_t k = 1; k < read->level_count; k++) {
		entry->level_count = k + 1;
		entry->levels[k].bound = fminf(error_bound(entry, read, k),
					       entry->levels[k - 1].bound);
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
				 voe_base_arena *scratch,
				 const voe_assets_model *model,
				 const voe_3d_model_bake *bake,
				 const uint8_t *glb, size_t glb_size,
				 const uint8_t *levels, size_t levels_size,
				 voe_3d_model_entry *entry,
				 voe_base_error *error)
{
	const uint32_t own = voe_3d_model_bake_levels(model);
	voe_3d_model_levels read = {};
	bool fresh;

	VOE_BASE_ASSERT(device != NULL && scratch != NULL,
			"uploading levels with no device or arena");
	VOE_BASE_ASSERT(model != NULL && bake != NULL && entry != NULL,
			"uploading the levels of nothing");
	VOE_BASE_ASSERT(entry->level_count == 1, "levels uploaded twice");

	fresh = levels != NULL &&
		voe_3d_model_levels_read(scratch, levels, levels_size, glb,
					 glb_size, &read);
	if (own > 1)
		return upload_own(device, scratch, model, bake, own,
				  fresh && read.own && read.level_count == own ?
					  &read :
					  NULL,
				  entry, error);
	if (!fresh || !fits(&read, bake))
		return true;
	return upload_made(device, &read, entry, error);
}
