// A prepared `.glb`'s cooked copy (3d/model_cooked.h): the write and the read
// of the layout that header gives, in its order.
//
// THE WRITE WALKS TWICE: once measuring, `at` NULL, then filling the bytes it
// measured, pushed once and zeroed, so a gap before a section is zero and the
// walk that sizes the copy is the walk that writes it. The pictures are mipped
// and cooked before either walk.
//
// THE READ IS A CURSOR that turns false at the first count past its limit or
// section past the bytes, every later take then giving nothing, so each step
// reads on and the end checks once. A section is a pointer into the bytes; a
// scalar is copied out, as it may sit at any offset. Every index is checked
// below what it indexes, and every part's range within its bake, so the put
// and a ray over the shape stay inside the arrays. A part wearing glTF's
// default needs a primitive that does, or the upload would make none.
#include "model_bake.h"
#include "models_store.h"

#include <3d/model_cooked.h>
#include <3d/model_levels.h>
#include <3d/shape_geometry.h>
#include <assets/image.h>
#include <assets/model.h>
#include <base/arena.h>
#include <base/assert.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// "VOEM" in a little-endian word.
#define MAGIC 0x4D454F56u
#define VERSION 1u
#define ALIGN 16u

// Where a write is: `at` NULL while it measures.
typedef struct {
	uint8_t *at;
	size_t used;
} writer;

static void put(writer *w, const void *from, size_t size)
{
	if (w->at != NULL && size > 0)
		memcpy(w->at + w->used, from, size);
	w->used += size;
}

static void put_u32(writer *w, uint32_t value)
{
	put(w, &value, sizeof(value));
}

// `size` bytes of `from` at the next multiple of ALIGN.
static void put_section(writer *w, const void *from, size_t size)
{
	w->used = (w->used + ALIGN - 1) & ~(size_t)(ALIGN - 1);
	put(w, from, size);
}

static void put_bake(writer *w, const voe_3d_model_bake *bake)
{
	put_u32(w, bake->vertex_count);
	put_u32(w, bake->index_count);
	put_u32(w, bake->part_count);
	put_section(w, bake->parts, bake->part_count * sizeof(*bake->parts));
	put_section(w, bake->vertices,
		    (size_t)bake->vertex_count * sizeof(*bake->vertices));
	put_section(w, bake->indices,
		    (size_t)bake->index_count * sizeof(*bake->indices));
}

static void put_shape(writer *w, const voe_3d_shape_geometry *shape)
{
	put_u32(w, shape->vertex_count);
	put_u32(w, shape->index_count);
	put_u32(w, shape->edge_count);
	put_section(w, shape->vertices,
		    (size_t)shape->vertex_count * sizeof(*shape->vertices));
	put_section(w, shape->indices,
		    (size_t)shape->index_count * sizeof(*shape->indices));
	put_section(w, shape->edges,
		    (size_t)shape->edge_count * sizeof(*shape->edges));
}

static void put_levels(writer *w, const voe_3d_model_levels *levels)
{
	put_u32(w, levels->own);
	put_u32(w, levels->level_count);
	put_u32(w, levels->part_count);
	put_section(w, levels->errors, sizeof(levels->errors));
	put_section(w, levels->swaps, sizeof(levels->swaps));
	put_section(w, levels->index_counts, sizeof(levels->index_counts));
	for (uint32_t k = 1; k < levels->level_count; k++) {
		for (uint32_t p = 0; p < levels->part_count; p++)
			put_section(w, levels->indices[k][p],
				    (size_t)levels->index_counts[k][p] *
					    sizeof(uint32_t));
	}
}

// The whole copy, `total` its size in the head: 0 while measuring.
static void put_copy(writer *w, const voe_3d_model_prepared *prepared,
		     const voe_assets_bytes *pictures,
		     voe_3d_model_cooked_key key, uint64_t total)
{
	const voe_assets_model *model = prepared->model;
	const voe_3d_model_levels none = { 0 };

	put_u32(w, MAGIC);
	put_u32(w, VERSION);
	put(w, &key.glb, sizeof(key.glb));
	put(w, &key.parts, sizeof(key.parts));
	put(w, &total, sizeof(total));
	VOE_BASE_ASSERT(w->used == VOE_3D_MODEL_COOKED_HEAD, "a head of its size");

	put_u32(w, model->material_count);
	put_section(w, model->materials,
		    model->material_count * sizeof(*model->materials));
	put_u32(w, model->primitive_count);
	put_section(w, NULL, 0);
	for (uint32_t i = 0; i < model->primitive_count; i++)
		put_u32(w, model->primitives[i].material);
	put_u32(w, model->image_count);
	for (uint32_t i = 0; i < model->image_count; i++)
		put_section(w, pictures[i].bytes, pictures[i].count);

	put_bake(w, &prepared->bake);
	put_shape(w, &prepared->shape);
	put(w, &prepared->centre, sizeof(prepared->centre));
	put(w, &prepared->radius, sizeof(prepared->radius));
	put_u32(w, prepared->own);
	put_u32(w, prepared->measured);
	put_u32(w, prepared->made);
	put_u32(w, prepared->own_count);
	for (uint32_t k = 1; k < prepared->own_count; k++)
		put_bake(w, &prepared->own_levels[k]);
	put_levels(w, prepared->made || prepared->measured ? &prepared->read :
							     &none);
}

// The kind `image`'s first use in `model`'s materials says, COLOUR for none.
static voe_assets_image_kind kind_of(const voe_assets_model *model,
				     uint32_t image)
{
	for (uint32_t i = 0; i < model->material_count; i++) {
		const voe_assets_material *m = &model->materials[i];

		if (m->base_colour_image == image || m->emissive_image == image)
			return VOE_ASSETS_IMAGE_COLOUR;
		if (m->normal_image == image)
			return VOE_ASSETS_IMAGE_NORMAL;
		if (m->metallic_roughness_image == image ||
		    m->occlusion_image == image)
			return VOE_ASSETS_IMAGE_DATA;
	}
	return VOE_ASSETS_IMAGE_COLOUR;
}

voe_assets_bytes voe_3d_model_cooked_write(voe_base_arena *arena,
					   const voe_3d_model_prepared *prepared,
					   voe_3d_model_cooked_key key)
{
	VOE_BASE_ASSERT(arena != NULL && prepared != NULL,
			"cooking nothing, or onto no arena");
	const voe_assets_model *model = prepared->model;

	VOE_BASE_ASSERT(model->image_count == 0 || model->images != NULL,
			"cooking a model whose pictures are not decoded");

	voe_assets_bytes *pictures = voe_base_arena_push(
		arena, (model->image_count + 1) * sizeof(*pictures));
	writer w = { 0 };
	size_t size;

	for (uint32_t i = 0; i < model->image_count; i++) {
		const voe_assets_image_kind kind = kind_of(model, i);
		const voe_assets_image mipped =
			voe_assets_image_mips(arena, &model->images[i], kind);

		pictures[i] = voe_assets_image_cooked_write(arena, &mipped,
							    kind, 0);
	}
	put_copy(&w, prepared, pictures, key, 0);
	size = w.used;
	w = (writer){ .at = voe_base_arena_push(arena, size) };
	put_copy(&w, prepared, pictures, key, size);
	VOE_BASE_ASSERT(w.used == size, "a copy written as measured");
	return (voe_assets_bytes){ .bytes = w.at, .count = size };
}

bool voe_3d_model_cooked_key_of(const uint8_t *head, size_t size,
				voe_3d_model_cooked_key *key)
{
	uint32_t word[2];

	VOE_BASE_ASSERT(head != NULL || size == 0, "a head with no bytes");
	VOE_BASE_ASSERT(key != NULL, "a key read into nothing");

	if (size < VOE_3D_MODEL_COOKED_HEAD)
		return false;
	memcpy(word, head, sizeof(word));
	if (word[0] != MAGIC || word[1] != VERSION)
		return false;
	memcpy(&key->glb, head + 8, sizeof(key->glb));
	memcpy(&key->parts, head + 16, sizeof(key->parts));
	return true;
}

// Where a read is; `ok` false from the first step past the bytes or a limit.
typedef struct {
	const uint8_t *bytes;
	size_t size;
	size_t used;
	bool ok;
} reader;

// `size` bytes at the cursor; NULL, `ok` false, past the end.
static const void *take(reader *r, size_t size)
{
	const uint8_t *at = r->bytes + r->used;

	if (!r->ok || size > r->size - r->used) {
		r->ok = false;
		return NULL;
	}
	r->used += size;
	return at;
}

static uint32_t take_u32(reader *r)
{
	const void *at = take(r, sizeof(uint32_t));
	uint32_t value = 0;

	if (at != NULL)
		memcpy(&value, at, sizeof(value));
	return value;
}

// A section's bytes at the next multiple of ALIGN; NULL when empty or short.
static const void *take_section(reader *r, size_t size)
{
	const size_t aligned = (r->used + ALIGN - 1) & ~(size_t)(ALIGN - 1);
	const void *at;

	if (aligned > r->size)
		r->ok = false;
	else
		r->used = aligned;
	at = take(r, size);
	return size == 0 ? NULL : at;
}

// `ok` false unless `holds`.
static void need(reader *r, bool holds)
{
	r->ok = r->ok && holds;
}

// Whether every one of `count` indices is below `limit`.
static bool below(const uint32_t *indices, uint32_t count, uint32_t limit)
{
	for (uint32_t i = 0; i < count; i++) {
		if (indices[i] >= limit)
			return false;
	}
	return true;
}

// A bake whose every part lies within it and indexes its own vertices.
static void take_bake(reader *r, voe_3d_model_bake *bake)
{
	const voe_3d_model_bake_part *parts;

	bake->vertex_count = take_u32(r);
	bake->index_count = take_u32(r);
	bake->part_count = take_u32(r);
	need(r, bake->part_count <= VOE_3D_MODEL_PARTS);
	if (!r->ok)
		return;
	parts = take_section(r, bake->part_count * sizeof(*parts));
	bake->vertices = (voe_render_vertex *)take_section(
		r, (size_t)bake->vertex_count * sizeof(*bake->vertices));
	bake->indices = (uint32_t *)take_section(
		r, (size_t)bake->index_count * sizeof(*bake->indices));
	for (uint32_t p = 0; r->ok && p < bake->part_count; p++) {
		const voe_3d_model_bake_part *part = &parts[p];

		need(r, (uint64_t)part->first_vertex + part->vertex_count <=
					bake->vertex_count &&
				(uint64_t)part->first_index + part->index_count <=
					bake->index_count);
		need(r, r->ok && below(bake->indices + part->first_index,
				       part->index_count, part->vertex_count));
		bake->parts[p] = *part;
	}
}

static void take_shape(reader *r, voe_3d_shape_geometry *shape)
{
	shape->vertex_count = take_u32(r);
	shape->index_count = take_u32(r);
	shape->edge_count = take_u32(r);
	shape->vertices = take_section(
		r, (size_t)shape->vertex_count * sizeof(*shape->vertices));
	shape->indices = take_section(
		r, (size_t)shape->index_count * sizeof(*shape->indices));
	shape->edges = take_section(
		r, (size_t)shape->edge_count * sizeof(*shape->edges));
	need(r, r->ok && below(shape->indices, shape->index_count,
			       shape->vertex_count));
}

// The levels read, each made level's indices below its level-0 part's
// vertices in `bake`.
static void take_levels(reader *r, const voe_3d_model_bake *bake,
			voe_3d_model_levels *levels)
{
	const void *errors, *swaps, *counts;

	levels->own = take_u32(r) != 0;
	levels->level_count = take_u32(r);
	levels->part_count = take_u32(r);
	need(r, levels->level_count <= VOE_3D_MODEL_LEVELS &&
			levels->part_count <= bake->part_count);
	errors = take_section(r, sizeof(levels->errors));
	swaps = take_section(r, sizeof(levels->swaps));
	counts = take_section(r, sizeof(levels->index_counts));
	if (!r->ok)
		return;
	memcpy(levels->errors, errors, sizeof(levels->errors));
	memcpy(levels->swaps, swaps, sizeof(levels->swaps));
	memcpy(levels->index_counts, counts, sizeof(levels->index_counts));
	for (uint32_t k = 1; k < levels->level_count; k++) {
		for (uint32_t p = 0; p < levels->part_count; p++) {
			const uint32_t count = levels->index_counts[k][p];

			levels->indices[k][p] = (uint32_t *)take_section(
				r, (size_t)count * sizeof(uint32_t));
			need(r, r->ok && below(levels->indices[k][p], count,
					       bake->parts[p].vertex_count));
		}
	}
}

// The materials, each primitive's material and the pictures into `model`,
// its two arrays that hold pointers on `arena`.
static void take_model(reader *r, voe_base_arena *arena,
		       voe_assets_model *model)
{
	voe_assets_primitive *primitives;
	voe_assets_image *images;

	model->material_count = take_u32(r);
	model->materials = take_section(
		r, model->material_count * sizeof(*model->materials));
	model->primitive_count = take_u32(r);
	(void)take_section(r, 0);
	need(r, model->primitive_count <=
			(r->size - r->used) / sizeof(uint32_t));
	if (!r->ok)
		return;
	primitives = voe_base_arena_push(
		arena, (model->primitive_count + 1) * sizeof(*primitives));
	for (uint32_t i = 0; i < model->primitive_count; i++)
		primitives[i].material = take_u32(r);
	model->primitives = primitives;

	model->image_count = take_u32(r);
	need(r, model->image_count <= (r->size - r->used) /
					      VOE_ASSETS_IMAGE_COOKED_HEAD);
	if (!r->ok)
		return;
	images = voe_base_arena_push(arena,
				     (model->image_count + 1) * sizeof(*images));
	for (uint32_t i = 0; r->ok && i < model->image_count; i++) {
		voe_assets_image_kind kind;
		uint64_t key;
		size_t used = 0;

		(void)take_section(r, 0);
		need(r, r->ok && voe_assets_image_cooked_read(
					 r->bytes + r->used, r->size - r->used,
					 &images[i], &kind, &key, &used));
		(void)take(r, used);
	}
	model->images = images;
}

// Whether every level-0 part wears a material the upload will have: one of
// the file's, or glTF's default, made when some primitive names none.
static bool parts_wear_known(const voe_3d_model_prepared *prepared)
{
	const voe_assets_model *model = prepared->model;
	bool fallback = false;

	for (uint32_t i = 0; i < model->primitive_count; i++)
		fallback = fallback ||
			   model->primitives[i].material >= model->material_count;
	for (uint32_t p = 0; p < prepared->bake.part_count; p++) {
		if (prepared->bake.parts[p].material >= model->material_count &&
		    !fallback)
			return false;
	}
	return true;
}

// Everything after the head into `prepared` and `model`; false at the first
// step that does not hold.
static bool take_copy(reader *r, voe_base_arena *arena,
		      voe_3d_model_prepared *prepared, voe_assets_model *model)
{
	const void *at;

	take_model(r, arena, model);
	take_bake(r, &prepared->bake);
	take_shape(r, &prepared->shape);
	at = take(r, sizeof(prepared->centre));
	if (at != NULL)
		memcpy(&prepared->centre, at, sizeof(prepared->centre));
	at = take(r, sizeof(prepared->radius));
	if (at != NULL)
		memcpy(&prepared->radius, at, sizeof(prepared->radius));
	prepared->own = take_u32(r) != 0;
	prepared->measured = take_u32(r) != 0;
	prepared->made = take_u32(r) != 0;
	prepared->own_count = take_u32(r);
	need(r, prepared->own_count <= VOE_3D_MODEL_LEVELS);
	for (uint32_t k = 1; r->ok && k < prepared->own_count; k++)
		take_bake(r, &prepared->own_levels[k]);
	take_levels(r, &prepared->bake, &prepared->read);
	need(r, !prepared->made ||
			prepared->read.part_count == prepared->bake.part_count);
	return r->ok && r->used == r->size && parts_wear_known(prepared);
}

const voe_3d_model_prepared *voe_3d_model_cooked_read(voe_base_arena *arena,
						       const uint8_t *bytes,
						       size_t size,
						       voe_3d_model_cooked_key *key)
{
	VOE_BASE_ASSERT(arena != NULL && key != NULL,
			"reading a copy onto no arena, or its key into nothing");
	VOE_BASE_ASSERT(bytes != NULL && (uintptr_t)bytes % ALIGN == 0,
			"a copy's bytes not 16-aligned");

	const struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
	voe_3d_model_prepared *prepared;
	voe_assets_model *model;
	voe_3d_model_cooked_key head;
	reader r = { .bytes = bytes, .size = size,
		     .used = VOE_3D_MODEL_COOKED_HEAD, .ok = true };
	uint64_t total;

	if (!voe_3d_model_cooked_key_of(bytes, size, &head))
		return NULL;
	memcpy(&total, bytes + 24, sizeof(total));
	if (total != size)
		return NULL;
	prepared = voe_base_arena_push(arena, sizeof(*prepared));
	model = voe_base_arena_push(arena, sizeof(*model));
	prepared->model = model;
	if (!take_copy(&r, arena, prepared, model)) {
		voe_base_arena_rewind(arena, mark);
		return NULL;
	}
	*key = head;
	return prepared;
}
