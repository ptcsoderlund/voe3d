// The model store: a fixed table of entries, each with an arena of its own, and
// the load that reads, bakes, uploads and keeps one file.
//
// A LOAD IS FOUR STEPS, in the order ids are needed: `assets` reads the bytes
// into a scratch arena, model_bake.h flattens them into parts, model_upload.h
// puts the pictures and materials on the card, and each part becomes a geometry.
// The CPU copy of the whole model, `shape`, is built last into the entry's own
// arena. The scratch arena is the load's and is destroyed before it returns.
//
// A FAILED LOAD GIVES BACK WHAT IT UPLOADED, every texture, shading record and
// geometry it had made, so that a file failing again and again does not fill
// the device; its entry keeps the path and the stamp and nothing else.
//
// EACH ENTRY OWNS AN ARENA holding its path and its shape, so one entry's memory
// goes back in one call without touching another's.
//
// CONSTRAINTS: _find is a scan over the entries by string compare, which at
// VOE_3D_MODELS entries is nothing; a hash of the path would lift it if the
// store ever grows by orders of magnitude. A full store refuses a new path, and
// _fail says so on stderr rather than keeping it.
#include "model_bake.h"
#include "model_upload.h"

#include <3d/models.h>
#include <assets/model.h>
#include <base/arena.h>
#include <base/assert.h>
#include <base/report.h>

#include <stdlib.h>
#include <string.h>

// An entry's arena starts this big and grows; a load's scratch arena too,
// since a decoded picture alone may be many megabytes.
#define ENTRY_BLOCK (64 * 1024)
#define SCRATCH_BLOCK (1024 * 1024)

struct voe_3d_models {
	voe_3d_model_entry entries[VOE_3D_MODELS];
	voe_base_arena *memory[VOE_3D_MODELS];
	uint32_t count;
};

voe_3d_models *voe_3d_models_new(void)
{
	voe_3d_models *models = calloc(1, sizeof(*models));

	VOE_BASE_ASSERT(models != NULL, "out of memory making a model store");
	return models;
}

void voe_3d_models_destroy(voe_3d_models *models)
{
	if (models == NULL)
		return;
	for (uint32_t i = 0; i < models->count; i++)
		voe_base_arena_destroy(models->memory[i]);
	free(models);
}

// `path`'s index, or VOE_3D_MODELS when the store does not hold it.
static uint32_t find(const voe_3d_models *models, const char *path)
{
	for (uint32_t i = 0; i < models->count; i++) {
		if (strcmp(models->entries[i].path, path) == 0)
			return i;
	}
	return VOE_3D_MODELS;
}

// A failed entry for `path` with its own arena; NULL when the store is full.
static voe_3d_model_entry *entry_new(voe_3d_models *models, const char *path,
				     uint64_t stamp)
{
	size_t length = strlen(path) + 1;
	voe_base_arena *memory;
	char *copy;

	VOE_BASE_ASSERT(find(models, path) == VOE_3D_MODELS,
			"loading a model path the store already holds");

	if (models->count == VOE_3D_MODELS) {
		VOE_BASE_ERROR("3d",
			       "no room for %s — the store holds %u models",
			       path, VOE_3D_MODELS);
		return NULL;
	}

	memory = voe_base_arena_new(ENTRY_BLOCK);
	copy = voe_base_arena_push(memory, length);
	memcpy(copy, path, length);

	models->memory[models->count] = memory;
	models->entries[models->count] = (voe_3d_model_entry){
		.path = copy,
		.stamp = stamp,
	};
	return &models->entries[models->count++];
}

// The whole model once more on the CPU, in the entry's arena: the bake's
// vertices, and its indices counted from the model's first vertex.
static void build_shape(voe_base_arena *memory, const voe_3d_model_bake *bake,
			voe_3d_shape_geometry *out)
{
	voe_render_vertex *vertices;
	uint32_t *indices;

	if (bake->part_count == 0)
		return;

	vertices = voe_base_arena_push(memory, (size_t)bake->vertex_count *
						       sizeof(*vertices));
	indices = voe_base_arena_push(memory, (size_t)bake->index_count *
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
	voe_3d_shape_geometry_build(memory, vertices, bake->vertex_count,
				    indices, bake->index_count, out);
}

// Every part a geometry; false at the first the device has no room for, with
// `made` saying how many were.
static bool upload_parts(voe_render_device *device,
			 const voe_3d_model_bake *bake,
			 const voe_3d_model_upload *upload,
			 voe_3d_model_entry *entry, uint32_t *made,
			 voe_base_error *error)
{
	for (uint32_t p = 0; p < bake->part_count; p++) {
		const voe_3d_model_bake_part *part = &bake->parts[p];

		if (!voe_render_geometry_create(
			    device, bake->vertices + part->first_vertex,
			    part->vertex_count,
			    bake->indices + part->first_index,
			    part->index_count, &entry->parts[p].geometry,
			    error))
			return false;
		entry->parts[p].material =
			voe_3d_model_upload_material(upload, part->material);
		*made = p + 1;
	}
	return true;
}

// What a failed load had put on the card, given back.
static void give_back(voe_render_device *device,
		      const voe_3d_model_upload *upload,
		      const voe_3d_model_entry *entry, uint32_t geometries)
{
	for (uint32_t i = 0; i < geometries; i++)
		(void)voe_render_geometry_destroy(device,
						  entry->parts[i].geometry);
	for (uint32_t i = 0; i < upload->shading_count; i++)
		(void)voe_render_shading_destroy(device, upload->shadings[i]);
	for (uint32_t i = 0; i < upload->texture_count; i++)
		(void)voe_render_texture_destroy(device, upload->textures[i]);
}

bool voe_3d_models_load(voe_3d_models *models, voe_render_device *device,
			const char *path, uint64_t stamp, const uint8_t *bytes,
			size_t size, voe_base_error *error)
{
	voe_3d_model_upload upload = { 0 };
	voe_3d_model_entry *entry;
	voe_3d_model_bake bake;
	voe_assets_model model;
	voe_base_arena *scratch;
	uint32_t made = 0;
	bool loaded;

	VOE_BASE_ASSERT(models != NULL, "loading a model into no store");
	VOE_BASE_ASSERT(device != NULL, "loading a model with no device");
	VOE_BASE_ASSERT(path != NULL, "loading a model with no path");

	entry = entry_new(models, path, stamp);
	if (entry == NULL) {
		if (error != NULL)
			*error = VOE_BASE_ERROR_REFUSED;
		return false;
	}

	scratch = voe_base_arena_new(SCRATCH_BLOCK);
	loaded = voe_assets_model_read_glb(bytes, size, scratch, &model,
					   error) &&
		 voe_3d_model_bake_create(scratch, &model, &bake, error) &&
		 voe_3d_model_upload_create(device, scratch, &model, &upload,
					    error) &&
		 upload_parts(device, &bake, &upload, entry, &made, error);

	if (loaded) {
		build_shape(models->memory[entry - models->entries], &bake,
			    &entry->shape);
		entry->part_count = bake.part_count;
		entry->loaded = true;
	} else {
		VOE_BASE_ERROR("3d", "could not load the model %s", path);
		give_back(device, &upload, entry, made);
	}

	voe_base_arena_destroy(scratch);
	return loaded;
}

void voe_3d_models_fail(voe_3d_models *models, const char *path,
			uint64_t stamp)
{
	VOE_BASE_ASSERT(models != NULL, "failing a model in no store");
	VOE_BASE_ASSERT(path != NULL, "failing a model with no path");

	(void)entry_new(models, path, stamp);
}

const voe_3d_model_entry *voe_3d_models_find(const voe_3d_models *models,
					     const char *path)
{
	uint32_t index;

	VOE_BASE_ASSERT(models != NULL, "finding a model in no store");
	VOE_BASE_ASSERT(path != NULL, "finding a model with no path");

	index = find(models, path);
	return index == VOE_3D_MODELS ? NULL : &models->entries[index];
}

uint32_t voe_3d_models_count(const voe_3d_models *models)
{
	VOE_BASE_ASSERT(models != NULL, "counting a store that is not there");

	return models->count;
}

const voe_3d_model_entry *voe_3d_models_at(const voe_3d_models *models,
					   uint32_t index)
{
	VOE_BASE_ASSERT(models != NULL, "reading a store that is not there");
	VOE_BASE_ASSERT(index < models->count, "a model past the store's end");

	return &models->entries[index];
}
