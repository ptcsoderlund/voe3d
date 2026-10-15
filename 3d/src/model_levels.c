// The levels file made and read (3d/model_levels.h): a `.glb` parsed, baked at
// level 0 and simplified a part at a time into the file's bytes, and those
// bytes read back field by field against the `.glb` they were made from; and a
// view's level of a loaded model, walked from the top over bounds made
// non-increasing at load.
//
// A LEVEL IS MADE WHOLE BEFORE IT IS JUDGED: every part simplified, its
// triangles summed, and only then held against three quarters of the level
// before. A dropped level's indices are given back to the arena at once.
//
// A PART'S TARGET IS ITS OWN TRIANGLES SHIFTED, so every part keeps its share of
// the model and a small part may reach nought triangles, which draws nothing at
// that level; the radius bound usually stops it first.
//
// THE FILE IS WRITTEN AND READ WITH memcpy at a byte offset, since nothing in it
// is aligned past four bytes and the hash sits at eight: native-endian, as 0426
// allows, a file made on one machine read on the same kind.
#include <3d/model_levels.h>

#include "model_bake.h"
#include "simplify.h"

#include <assets/model.h>
#include <base/assert.h>

#include <math.h>
#include <string.h>

#define MAGIC "VOEL"
#define VERSION 1u
// Magic, version, hash, own, level count, part count.
#define HEAD_BYTES (4 + 4 + 8 + 4 + 4 + 4)

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

// Level k of every part, from level 0, into `levels`; false when it holds more
// than three quarters of `previous` triangles, its indices then given back.
static bool simplify_level(voe_base_arena *arena, const voe_3d_model_bake *bake,
			   uint32_t k, float max_error, uint64_t *previous,
			   voe_3d_model_levels *levels)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
	uint64_t triangles = 0;
	float error = 0.0f;

	for (uint32_t p = 0; p < bake->part_count; p++) {
		const voe_3d_model_bake_part *part = &bake->parts[p];
		uint32_t *out = voe_base_arena_push(
			arena, (size_t)part->index_count * sizeof(*out));
		float part_error = 0.0f;
		uint32_t count = voe_3d_simplify(
			arena, bake->vertices + part->first_vertex,
			part->vertex_count, bake->indices + part->first_index,
			part->index_count, (part->index_count / 3 >> k) * 3,
			max_error, 0.0f, out, &part_error);

		levels->indices[k][p] = out;
		levels->index_counts[k][p] = count;
		triangles += count / 3;
		error = fmaxf(error, part_error);
	}

	if (triangles * 4 > *previous * 3) {
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

static void simplify_levels(voe_base_arena *arena,
			    const voe_3d_model_bake *bake,
			    voe_3d_model_levels *levels)
{
	const float max_error = radius_of(bake) / 4.0f;
	uint64_t previous = bake->index_count / 3;

	levels->part_count = bake->part_count;
	for (uint32_t k = 1; k < VOE_3D_MODEL_LEVELS && previous > 0; k++) {
		if (!simplify_level(arena, bake, k, max_error, &previous, levels))
			break;
		levels->level_count = k + 1;
	}
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
					size_t *size, voe_base_error *error)
{
	voe_3d_model_levels levels = { .level_count = 1 };
	voe_assets_model model;
	voe_3d_model_bake bake;

	VOE_BASE_ASSERT(arena != NULL, "making levels without an arena");
	VOE_BASE_ASSERT(glb != NULL && size != NULL, "making levels of nothing");

	if (!voe_assets_model_read_glb(glb, glb_size, arena, &model, error))
		return NULL;
	if (voe_3d_model_bake_levels(&model) > 1) {
		levels.own = true;
	} else {
		if (!voe_3d_model_bake_create(arena, &model, 0, &bake, error))
			return NULL;
		simplify_levels(arena, &bake, &levels);
	}
	return write_file(arena, &levels, voe_3d_model_levels_hash(glb, glb_size),
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

// The magic, version, hash and counts, each within its limit.
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
	    !take_u32(reader, &out->part_count))
		return false;
	out->own = own == 1;
	return out->level_count >= 1 &&
	       out->level_count <= VOE_3D_MODEL_LEVELS &&
	       out->part_count <= VOE_3D_MODEL_PARTS &&
	       (!out->own || (out->level_count == 1 && out->part_count == 0));
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

uint32_t voe_3d_model_level(const voe_3d_model_entry *model,
			    const voe_scene_transform *placed,
			    voe_math_double3 eye, const voe_render_view *view)
{
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
		return 0;

	s = radius * fabsf(view->projection.m[1][1]) / clip.w;
	for (uint32_t k = model->level_count - 1; k > 0; k--)
		if (model->levels[k].bound > s)
			return k;
	return 0;
}
