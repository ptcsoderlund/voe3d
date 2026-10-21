// A read `.glb` prepared for the store's put (3d/model_prepare.h): level 0
// baked, the file's own levels baked or its levels file read and checked, the
// CPU shape built and the sphere measured, all on the caller's arena. models.c
// puts what this makes; models_levels.c uploads its levels.
//
// OWN LEVELS ARE BAKED IN ORDER UNTIL ONE MATCHES NO PART: it would draw
// nothing, and the levels after it are farther still, so they are not baked.
// A bake that fails fails the prepare, as it failed the load.
//
// A STALE OR WRONG LEVELS FILE LOADS LEVEL 0 ALONE (0426 point 1): a read
// saying false, a part count not the bake's, or any index at or past its part's
// vertices, all found here, before anything is uploaded. An own file's errors
// and swaps are kept only when the fresh file is own and counts the bake's own
// levels.
//
// NOTHING HERE IS SHARED: no device, no store, no static state, so it runs on
// any thread with an arena of its own.
#include "model_bake.h"
#include "models_store.h"

#include <3d/model_levels.h>
#include <3d/model_prepare.h>
#include <3d/shape_geometry.h>
#include <base/arena.h>
#include <base/assert.h>

#include <math.h>
#include <string.h>

uint32_t voe_3d_models_part_of(const voe_3d_model_bake *bake, uint32_t material)
{
	for (uint32_t p = 0; p < bake->part_count; p++) {
		if (bake->parts[p].material == material)
			return p;
	}
	return VOE_3D_MODEL_PARTS;
}

// The bake's box's centre and the farthest vertex from it; untouched for a
// bake with no vertices.
static void measure_sphere(voe_3d_model_prepared *prepared)
{
	const voe_3d_model_bake *bake = &prepared->bake;
	voe_math_float3 low = { INFINITY, INFINITY, INFINITY };
	voe_math_float3 high = { -INFINITY, -INFINITY, -INFINITY };
	float most = 0.0f;

	if (bake->vertex_count == 0)
		return;
	for (uint32_t v = 0; v < bake->vertex_count; v++) {
		low = voe_math_float3_min(low, bake->vertices[v].position);
		high = voe_math_float3_max(high, bake->vertices[v].position);
	}
	prepared->centre =
		voe_math_float3_scale(voe_math_float3_add(low, high), 0.5f);
	for (uint32_t v = 0; v < bake->vertex_count; v++)
		most = fmaxf(most, voe_math_float3_length(voe_math_float3_sub(
					   bake->vertices[v].position,
					   prepared->centre)));
	prepared->radius = most;
}

// The whole model once more on the CPU: the bake's vertices, and its indices
// counted from the model's first vertex.
static void build_shape(voe_base_arena *arena, const voe_3d_model_bake *bake,
			voe_3d_shape_geometry *out)
{
	voe_render_vertex *vertices;
	uint32_t *indices;

	if (bake->part_count == 0)
		return;

	vertices = voe_base_arena_push(arena, (size_t)bake->vertex_count *
						      sizeof(*vertices));
	indices = voe_base_arena_push(arena, (size_t)bake->index_count *
						     sizeof(*indices));
	memcpy(vertices, bake->vertices,
	       (size_t)bake->vertex_count * sizeof(*vertices));
	for (uint32_t p = 0; p < bake->part_count; p++) {
		const voe_3d_model_bake_part *part = &bake->parts[p];

		for (uint32_t i = 0; i < part->index_count; i++)
			indices[part->first_index + i] =
				bake->indices[part->first_index + i] +
				part->first_vertex;
	}
	voe_3d_shape_geometry_build(arena, vertices, bake->vertex_count,
				    indices, bake->index_count, out);
}

// Whether any part of `level` matches a level-0 part by material.
static bool matches(const voe_3d_model_bake *zero,
		    const voe_3d_model_bake *level)
{
	for (uint32_t q = 0; q < level->part_count; q++) {
		if (voe_3d_models_part_of(zero, level->parts[q].material) !=
		    VOE_3D_MODEL_PARTS)
			return true;
	}
	return false;
}

// The own levels 1 to `count` − 1 baked, at most VOE_3D_MODEL_LEVELS in all,
// stopping before the first that matches no part.
static bool bake_own(voe_base_arena *arena, const voe_assets_model *model,
		     uint32_t count, voe_3d_model_prepared *prepared,
		     voe_base_error *error)
{
	prepared->own_count = 1;
	for (uint32_t k = 1; k < count && k < VOE_3D_MODEL_LEVELS; k++) {
		if (!voe_3d_model_bake_create(arena, model, k,
					      &prepared->own_levels[k], error))
			return false;
		if (!matches(&prepared->bake, &prepared->own_levels[k]))
			break;
		prepared->own_count = k + 1;
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

const voe_3d_model_prepared *
voe_3d_model_prepare(voe_base_arena *arena, const voe_assets_model *model,
		     const uint8_t *glb, size_t glb_size, const uint8_t *levels,
		     size_t levels_size, voe_base_error *error)
{
	voe_3d_model_prepared *prepared;
	uint32_t own;
	bool fresh;

	VOE_BASE_ASSERT(arena != NULL, "preparing a model with no arena");
	VOE_BASE_ASSERT(model != NULL && glb != NULL,
			"preparing no model or no file");

	prepared = voe_base_arena_push(arena, sizeof(*prepared));
	prepared->model = model;
	if (!voe_3d_model_bake_create(arena, model, 0, &prepared->bake, error))
		return NULL;
	own = voe_3d_model_bake_levels(model);
	fresh = levels != NULL &&
		voe_3d_model_levels_read(arena, levels, levels_size, glb,
					 glb_size, &prepared->read);
	if (own > 1) {
		prepared->own = true;
		prepared->measured = fresh && prepared->read.own &&
				     prepared->read.level_count == own;
		if (!bake_own(arena, model, own, prepared, error))
			return NULL;
	} else {
		prepared->made = fresh && fits(&prepared->read, &prepared->bake);
	}
	build_shape(arena, &prepared->bake, &prepared->shape);
	measure_sphere(prepared);
	return prepared;
}
