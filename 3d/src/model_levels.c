// The levels file made and read (3d/model_levels.h): a `.glb` parsed with its
// pictures left encoded, baked at level 0 and simplified by level and part
// into the file's bytes, or its own
// levels baked and measured against level 0, and those bytes read back field
// by field against the `.glb` they were made from; each level's triangles; the
// bounds, capped and spread, and the metres one stands for; and a view's level
// of a loaded model, walked from the top over those bounds, with its share of
// the pixels in its band.
//
// AN OWN LEVEL IS BAKED AND GIVEN BACK ONE AT A TIME, so making the file holds
// level 0 and one own level at most; its error is measure.h's.
//
// THE LEVELS ARE SIMPLIFIED LEVEL BY LEVEL ON THE CALLER, each judged as it is
// made (0459 point 2): every part simplified into a buffer of its level-0 index
// count on the arena, the level's triangles summed and held against the level
// before. One not below it is rewound to its mark and ends the list (0436
// point 5), so at most one level's buffers are ever given back.
//
// A PART'S TARGET IS ITS OWN TRIANGLES × THE LEVEL'S SHARE ÷ 100, rounded down,
// so every part keeps its share of the model and a small part may reach nought
// triangles, which draws nothing at that level; the radius bound usually stops
// it first.
//
// THE FILE IS WRITTEN AND READ WITH memcpy at a byte offset, since nothing in it
// is aligned past four bytes and the hash sits at eight: native-endian, as 0426
// allows, a file made on one machine read on the same kind. The hash is 64-bit
// FNV-1a over the `.glb`'s bytes; the swaps sit at a fixed offset, so they are
// patched in place without the file being made again.
#include <3d/model_levels.h>

#include "measure.h"
#include "model_bake.h"
#include "simplify.h"

#include <assets/model.h>
#include <base/assert.h>

#include <math.h>
#include <string.h>

#define MAGIC "VOEL"
#define VERSION 5u
// Magic, version, hash, own, level count, part count; then the swaps of levels
// 1 to 5 at this fixed offset, so _swaps_set patches them in place.
#define SWAPS_AT (4 + 4 + 8 + 4 + 4 + 4)
#define SWAPS_BYTES ((VOE_3D_MODEL_LEVELS - 1) * sizeof(float))
#define HEAD_BYTES (SWAPS_AT + SWAPS_BYTES)

static_assert(VOE_ASSETS_LEVELS == VOE_3D_MODEL_LEVELS,
	      "a parts file's level count is the store's");

// Fixed, not a setting (0437 point 2): normals within this angle share a side,
// and the bounding radius divided by this is the most a collapse may cost.
#define SHARED_SIDE_DEGREES 60.0f
#define COLLAPSE_RADIUS_DIVISOR 4.0f

// A level's error kept under this share of half the screen (0425 point 3).
#define SCREEN_ERROR 0.002f
// No bound above the radius filling half the screen height (0500 point 5).
#define BOUND_CAP 1.0f
// Each level swaps in at least this many times as far as the one before
// (0495 point 1, 0500 point 2): the width of a dissolve band.
#define BOUND_SPREAD 1.25f

struct writer {
	uint8_t *bytes;
	size_t at;
};

struct reader {
	const uint8_t *bytes;
	size_t size;
	size_t at;
};

uint64_t voe_3d_model_levels_hash(const uint8_t *bytes, size_t size)
{
	uint64_t hash = 0xcbf29ce484222325ull;

	VOE_BASE_ASSERT(bytes != NULL || size == 0, "hashing nothing");
	for (size_t i = 0; i < size; i++) {
		hash ^= bytes[i];
		hash *= 0x100000001b3ull;
	}
	return hash;
}

// The farthest baked vertex from the centre of their box.
static float radius_of(const voe_3d_model_bake *bake)
{
	float low[3] = { INFINITY, INFINITY, INFINITY };
	float high[3] = { -INFINITY, -INFINITY, -INFINITY };
	float centre[3];
	float most = 0.0f;

	for (uint32_t v = 0; v < bake->vertex_count; v++) {
		const voe_math_float3 p = bake->vertices[v].position;
		const float at[3] = { p.x, p.y, p.z };

		for (int a = 0; a < 3; a++) {
			low[a] = fminf(low[a], at[a]);
			high[a] = fmaxf(high[a], at[a]);
		}
	}
	for (int a = 0; a < 3; a++)
		centre[a] = (low[a] + high[a]) * 0.5f;
	for (uint32_t v = 0; v < bake->vertex_count; v++) {
		const voe_math_float3 p = bake->vertices[v].position;
		const float x = p.x - centre[0];
		const float y = p.y - centre[1];
		const float z = p.z - centre[2];

		most = fmaxf(most, sqrtf(x * x + y * y + z * z));
	}
	return most;
}

// Level k of every part, from level 0, each part aiming at `share` percent of
// its triangles, into buffers on `arena`; false when it holds no fewer than
// `previous` triangles, its indices then given back to the arena.
static bool simplify_level(voe_base_arena *arena, const voe_3d_model_bake *bake,
			   uint32_t k, float share, float max_error,
			   uint64_t *previous, voe_3d_model_levels *levels)
{
	const struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
	uint64_t triangles = 0;
	float error = 0.0f;

	VOE_BASE_ASSERT(k >= 1 && k < VOE_3D_MODEL_LEVELS,
			"simplifying past the last level");
	VOE_BASE_ASSERT(share >= 0.0f && share <= 100.0f,
			"a level's share out of range");
	for (uint32_t p = 0; p < bake->part_count; p++) {
		const voe_3d_model_bake_part *part = &bake->parts[p];
		const uint32_t target =
			(uint32_t)((double)(part->index_count / 3) * share /
				   100.0) *
			3;
		uint32_t *out = voe_base_arena_push(
			arena, (size_t)part->index_count * sizeof(*out));
		float part_error = 0.0f;
		const uint32_t count = voe_3d_simplify(
			arena, bake->vertices + part->first_vertex,
			part->vertex_count, bake->indices + part->first_index,
			part->index_count, target, max_error,
			SHARED_SIDE_DEGREES, out, &part_error);

		levels->indices[k][p] = out;
		levels->index_counts[k][p] = count;
		triangles += count / 3;
		error = fmaxf(error, part_error);
	}

	if (triangles >= *previous) {
		memset(levels->indices[k], 0, sizeof(levels->indices[k]));
		memset(levels->index_counts[k], 0,
		       sizeof(levels->index_counts[k]));
		voe_base_arena_rewind(arena, mark);
		return false;
	}
	levels->errors[k] = error;
	*previous = triangles;
	return true;
}

// Levels 1 to `asked.count` − 1, made and judged in order on the caller: the
// first not below the one before in triangles ends the list.
static void simplify_levels(voe_base_arena *arena,
			    const voe_3d_model_bake *bake,
			    voe_assets_levels asked,
			    voe_3d_model_levels *levels)
{
	const float max_error = radius_of(bake) / COLLAPSE_RADIUS_DIVISOR;
	uint64_t previous = bake->index_count / 3;

	levels->part_count = bake->part_count;
	for (uint32_t k = 1; k < asked.count && previous > 0; k++) {
		if (!simplify_level(arena, bake, k, asked.shares[k], max_error,
				    &previous, levels))
			break;
		levels->level_count = k + 1;
	}
}

// `model`'s own levels after level 0, `zero`, each baked in turn until one
// fails or holds no triangle, as an own level matching no part ends the
// store's list: its triangles into `triangles`, and its error against `zero`
// into `errors` when not NULL. How many levels, level 0 included.
static uint32_t own_levels(voe_base_arena *arena, const voe_assets_model *model,
			   const voe_3d_model_bake *zero,
			   uint32_t triangles[VOE_3D_MODEL_LEVELS],
			   float errors[VOE_3D_MODEL_LEVELS])
{
	const uint32_t own = voe_3d_model_bake_levels(model);
	uint32_t count = 1;

	triangles[0] = zero->index_count / 3;
	for (uint32_t k = 1;
	     k < own && k < VOE_3D_MODEL_LEVELS && zero->index_count > 0; k++) {
		const struct voe_base_arena_mark mark =
			voe_base_arena_mark(arena);
		voe_3d_model_bake bake;

		if (!voe_3d_model_bake_create(arena, model, k, &bake, NULL) ||
		    bake.index_count == 0) {
			voe_base_arena_rewind(arena, mark);
			break;
		}
		triangles[k] = bake.index_count / 3;
		if (errors != NULL)
			errors[k] = voe_3d_measure_apart(arena, zero, &bake);
		voe_base_arena_rewind(arena, mark);
		count = k + 1;
	}
	return count;
}

static void put(struct writer *writer, const void *value, size_t size)
{
	memcpy(writer->bytes + writer->at, value, size);
	writer->at += size;
}

static void put_u32(struct writer *writer, uint32_t value)
{
	put(writer, &value, sizeof(value));
}

static const uint8_t *write_file(voe_base_arena *arena,
				 const voe_3d_model_levels *levels,
				 uint64_t hash, size_t *size)
{
	struct writer writer = { 0 };
	size_t total = HEAD_BYTES;

	for (uint32_t k = 1; k < levels->level_count; k++) {
		total += sizeof(float);
		for (uint32_t p = 0; p < levels->part_count; p++)
			total += sizeof(uint32_t) *
				 (1 + (size_t)levels->index_counts[k][p]);
	}
	writer.bytes = voe_base_arena_push(arena, total);

	put(&writer, MAGIC, 4);
	put_u32(&writer, VERSION);
	put(&writer, &hash, sizeof(hash));
	put_u32(&writer, levels->own ? 1 : 0);
	put_u32(&writer, levels->level_count);
	put_u32(&writer, levels->part_count);
	put(&writer, &levels->swaps[1], SWAPS_BYTES);
	for (uint32_t k = 1; k < levels->level_count; k++) {
		put(&writer, &levels->errors[k], sizeof(float));
		for (uint32_t p = 0; p < levels->part_count; p++) {
			const uint32_t count = levels->index_counts[k][p];

			put_u32(&writer, count);
			if (count > 0)
				put(&writer, levels->indices[k][p],
				    (size_t)count * sizeof(uint32_t));
		}
	}

	VOE_BASE_ASSERT(writer.at == total, "a levels file sized wrong");
	*size = total;
	return writer.bytes;
}

const uint8_t *voe_3d_model_levels_make(voe_base_arena *arena,
					const uint8_t *glb, size_t glb_size,
					voe_assets_levels levels, size_t *size,
					voe_base_error *error)
{
	voe_3d_model_levels made = { .level_count = 1 };
	voe_assets_model model;
	voe_3d_model_bake bake;

	VOE_BASE_ASSERT(arena != NULL, "making levels without an arena");
	VOE_BASE_ASSERT(glb != NULL && size != NULL, "making levels of nothing");
	VOE_BASE_ASSERT(levels.count >= 1 && levels.count <= VOE_ASSETS_LEVELS,
			"a level count out of range");

	memcpy(made.swaps, levels.swaps, sizeof(made.swaps));
	made.swaps[0] = 1.0f;
	for (uint32_t k = 1; k < VOE_3D_MODEL_LEVELS; k++)
		VOE_BASE_ASSERT(made.swaps[k] >= VOE_ASSETS_LEVELS_SWAP_LEAST &&
					made.swaps[k] <= VOE_ASSETS_LEVELS_SWAP_MOST,
				"a level swap out of range");
	if (!voe_assets_model_read_glb_encoded(glb, glb_size, arena, &model,
					       error) ||
	    !voe_3d_model_bake_create(arena, &model, 0, &bake, error))
		return NULL;
	if (voe_3d_model_bake_levels(&model) > 1) {
		uint32_t triangles[VOE_3D_MODEL_LEVELS];

		made.own = true;
		made.level_count =
			own_levels(arena, &model, &bake, triangles, made.errors);
	} else {
		simplify_levels(arena, &bake, levels, &made);
	}
	return write_file(arena, &made, voe_3d_model_levels_hash(glb, glb_size),
			  size);
}

// `size` bytes into `value`; false when fewer are left.
static bool take(struct reader *reader, void *value, size_t size)
{
	if (reader->size - reader->at < size)
		return false;
	if (size > 0)
		memcpy(value, reader->bytes + reader->at, size);
	reader->at += size;
	return true;
}

static bool take_u32(struct reader *reader, uint32_t *value)
{
	return take(reader, value, sizeof(*value));
}

// The magic, version, hash, counts and swaps, each within its limit.
static bool take_head(struct reader *reader, uint64_t glb_hash,
		      voe_3d_model_levels *out)
{
	char magic[4];
	uint32_t version;
	uint64_t hash;
	uint32_t own;

	if (!take(reader, magic, sizeof(magic)) ||
	    memcmp(magic, MAGIC, sizeof(magic)) != 0 ||
	    !take_u32(reader, &version) || version != VERSION ||
	    !take(reader, &hash, sizeof(hash)) || hash != glb_hash ||
	    !take_u32(reader, &own) || own > 1 ||
	    !take_u32(reader, &out->level_count) ||
	    !take_u32(reader, &out->part_count) ||
	    !take(reader, &out->swaps[1], SWAPS_BYTES))
		return false;
	out->own = own == 1;
	out->swaps[0] = 1.0f;
	for (uint32_t k = 1; k < VOE_3D_MODEL_LEVELS; k++) {
		if (!isfinite(out->swaps[k]) || out->swaps[k] <= 0.0f)
			return false;
	}
	return out->level_count >= 1 &&
	       out->level_count <= VOE_3D_MODEL_LEVELS &&
	       out->part_count <= VOE_3D_MODEL_PARTS &&
	       (!out->own || out->part_count == 0);
}

bool voe_3d_model_levels_read(voe_base_arena *arena, const uint8_t *bytes,
			      size_t size, const uint8_t *glb, size_t glb_size,
			      voe_3d_model_levels *out)
{
	struct reader reader = { .bytes = bytes, .size = size };

	VOE_BASE_ASSERT(arena != NULL && out != NULL, "reading levels into nothing");
	VOE_BASE_ASSERT(glb != NULL && (bytes != NULL || size == 0),
			"reading levels of nothing");

	*out = (voe_3d_model_levels){ 0 };
	if (!take_head(&reader, voe_3d_model_levels_hash(glb, glb_size), out))
		return false;

	for (uint32_t k = 1; k < out->level_count; k++) {
		if (!take(&reader, &out->errors[k], sizeof(float)) ||
		    !isfinite(out->errors[k]) || out->errors[k] < 0.0f)
			return false;
		for (uint32_t p = 0; p < out->part_count; p++) {
			uint32_t count;

			if (!take_u32(&reader, &count) || count % 3 != 0 ||
			    count > (reader.size - reader.at) / sizeof(uint32_t))
				return false;
			out->index_counts[k][p] = count;
			if (count == 0)
				continue;
			out->indices[k][p] = voe_base_arena_push(
				arena, (size_t)count * sizeof(uint32_t));
			if (!take(&reader, out->indices[k][p],
				  (size_t)count * sizeof(uint32_t)))
				return false;
		}
	}
	return reader.at == reader.size;
}

void voe_3d_model_levels_swaps_set(uint8_t *bytes, size_t size,
				   const float swaps[VOE_3D_MODEL_LEVELS])
{
	uint32_t version;

	VOE_BASE_ASSERT(bytes != NULL && swaps != NULL,
			"setting the swaps of nothing");
	VOE_BASE_ASSERT(size >= HEAD_BYTES, "a levels file shorter than its head");
	memcpy(&version, bytes + 4, sizeof(version));
	VOE_BASE_ASSERT(memcmp(bytes, MAGIC, 4) == 0 && version == VERSION,
			"setting the swaps of no version 5 levels file");
	for (uint32_t k = 1; k < VOE_3D_MODEL_LEVELS; k++)
		VOE_BASE_ASSERT(swaps[k] >= VOE_ASSETS_LEVELS_SWAP_LEAST &&
					swaps[k] <= VOE_ASSETS_LEVELS_SWAP_MOST,
				"a level swap out of range");
	memcpy(bytes + SWAPS_AT, &swaps[1], SWAPS_BYTES);
}

uint32_t voe_3d_model_levels_triangles(voe_base_arena *arena,
				       const uint8_t *glb, size_t glb_size,
				       const uint8_t *levels, size_t levels_size,
				       uint32_t triangles[VOE_3D_MODEL_LEVELS],
				       bool *own, bool *fresh)
{
	voe_3d_model_levels read = {};
	voe_assets_model model;
	voe_3d_model_bake zero;

	VOE_BASE_ASSERT(arena != NULL && glb != NULL,
			"counting the triangles of nothing");
	VOE_BASE_ASSERT(triangles != NULL && own != NULL && fresh != NULL,
			"counting triangles into nothing");

	*own = false;
	*fresh = levels != NULL &&
		 voe_3d_model_levels_read(arena, levels, levels_size, glb,
					  glb_size, &read);
	if (!voe_assets_model_read_glb_encoded(glb, glb_size, arena, &model,
					       NULL) ||
	    !voe_3d_model_bake_create(arena, &model, 0, &zero, NULL))
		return 0;
	if (voe_3d_model_bake_levels(&model) > 1) {
		*own = true;
		return own_levels(arena, &model, &zero, triangles, NULL);
	}
	triangles[0] = zero.index_count / 3;
	if (!*fresh || read.own)
		return 1;
	for (uint32_t k = 1; k < read.level_count; k++) {
		uint64_t indices = 0;

		for (uint32_t p = 0; p < read.part_count; p++)
			indices += read.index_counts[k][p];
		triangles[k] = (uint32_t)(indices / 3);
	}
	return read.level_count;
}

void voe_3d_model_levels_bounds(float radius,
				const float errors[VOE_3D_MODEL_LEVELS],
				const float swaps[VOE_3D_MODEL_LEVELS],
				uint32_t level_count, bool measured,
				float bounds[VOE_3D_MODEL_LEVELS])
{
	VOE_BASE_ASSERT(bounds != NULL, "bounds into nothing");
	VOE_BASE_ASSERT(!measured || (errors != NULL && swaps != NULL),
			"measured bounds of no errors or swaps");
	VOE_BASE_ASSERT(level_count <= VOE_3D_MODEL_LEVELS,
			"bounds of a level count out of range");

	if (level_count > 0)
		bounds[0] = INFINITY;
	for (uint32_t k = 1; k < level_count; k++) {
		float bound = ldexpf(1.0f, -(int)k);

		if (measured)
			bound = errors[k] > 0.0f ?
					SCREEN_ERROR * radius / errors[k] / swaps[k] :
					INFINITY;
		bounds[k] = fminf(fminf(bound, BOUND_CAP),
				  bounds[k - 1] / BOUND_SPREAD);
		VOE_BASE_ASSERT(!(bounds[k] > BOUND_CAP), "a bound past its cap");
	}
}

float voe_3d_model_level_metres(float radius, float bound, float fov_y)
{
	VOE_BASE_ASSERT(fov_y > 0.0f && fov_y < 3.14159265f,
			"metres through no lens");
	VOE_BASE_ASSERT(bound > 0.0f, "metres of no bound");

	if (!isfinite(bound))
		return 0.0f;
	return radius / tanf(fov_y * 0.5f) / bound;
}

// Level k's share of the pixels at `s`, from its band (0432 point 3): the
// band's floor, 0.8 of the bound, keeps it a fifth of the bound wide, narrow
// enough that the stipple shows only while a model rests in it.
static float shown_at(const voe_3d_model_entry *model, uint32_t k, float s)
{
	const float BAND_FLOOR = 0.8f;
	const float bound = model->levels[k].bound;
	float low;

	VOE_BASE_ASSERT(k < model->level_count, "a share of no level");
	if (k == 0 || !isfinite(bound))
		return 1.0f;
	low = BAND_FLOOR * bound;
	if (k + 1 < model->level_count)
		low = fmaxf(low, model->levels[k + 1].bound);
	if (s < low || bound <= low)
		return 1.0f;
	VOE_BASE_ASSERT(s < bound, "a level chosen above its bound");
	return (bound - s) / (bound - low);
}

voe_3d_model_level_choice
voe_3d_model_level_choose(const voe_3d_model_entry *model,
			  const voe_scene_transform *placed,
			  voe_math_double3 eye, const voe_render_view *view)
{
	const voe_3d_model_level_choice whole = { 0, 1.0f };
	voe_math_float3 scale;
	voe_math_float3 centre;
	voe_math_float4 clip;
	float radius;
	float s;

	VOE_BASE_ASSERT(model != NULL && placed != NULL && view != NULL,
			"choosing a level of nothing");
	VOE_BASE_ASSERT(model->level_count <= VOE_3D_MODEL_LEVELS,
			"a model with more levels than it can hold");

	scale = placed->scale;
	radius = model->radius *
		 fmaxf(fabsf(scale.x), fmaxf(fabsf(scale.y), fabsf(scale.z)));
	centre = voe_math_float4x4_transform_point(
		voe_scene_transform_matrix(*placed, eye), model->centre);
	clip = voe_math_float4x4_mul_float4(
		voe_math_float4x4_mul(view->projection, view->view),
		(voe_math_float4){ centre.x, centre.y, centre.z, 1.0f });
	if (model->level_count <= 1 || clip.w <= 0.0f ||
	    voe_math_float3_length(centre) <= radius)
		return whole;

	s = radius * fabsf(view->projection.m[1][1]) / clip.w;
	for (uint32_t k = model->level_count - 1; k > 0; k--)
		if (model->levels[k].bound > s)
			return (voe_3d_model_level_choice){
				k, shown_at(model, k, s)
			};
	return whole;
}

uint32_t voe_3d_model_level(const voe_3d_model_entry *model,
			    const voe_scene_transform *placed,
			    voe_math_double3 eye, const voe_render_view *view)
{
	const voe_3d_model_level_choice choice =
		voe_3d_model_level_choose(model, placed, eye, view);

	VOE_BASE_ASSERT(choice.shown > 0.0f && choice.shown <= 1.0f,
			"a share outside (0, 1]");
	return choice.level;
}
