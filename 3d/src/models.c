// The model store: a fixed table of entries, each with an arena of its own, and
// the load that reads, bakes, uploads and keeps one file.
//
// A `.glb` LOAD IS A PREPARE AND A PUT (0448 point 3). The prepare
// (3d/model_prepare.h) bakes the parts, the levels, the CPU shape and the
// sphere; the put, on the device's thread, has model_upload.h put the pictures
// and materials on the card, makes each part a geometry and, unless it is
// BLENDED, a blended twin record, has models_levels.c put the levels beside
// them, and copies the shape into the entry's own arena. _load_levelled reads
// with `assets`, prepares on a scratch arena of its own and puts, destroying
// the arena before it returns; the put's own scratch holds the upload's lists.
// Each part keeps its glTF material index as its source, so _parts_set can name
// what every part wears from a parts file without loading the model again.
//
// A FAILED LOAD GIVES BACK WHAT IT UPLOADED, every texture, shading record, twin
// and geometry it had made, so that a file failing again and again does not fill
// the device; its entry keeps the path and the stamp and nothing else. A
// failed load says its cause once, at the step that met it (0445): nothing
// here restates it or names the path, which is the caller's subject.
//
// EACH ENTRY OWNS AN ARENA holding its path, its shape and the lists of the
// textures and shading records its load made, so one entry's memory goes back
// in one call without touching another's.
//
// A PATH LOADED AGAIN IS LOADED INTO A NEW ENTRY FIRST, whole, beside the old
// one; only then does keep() swap it in and free the old, or, when it failed,
// give it back and move the old entry's stamp. So a replace needs room on the
// card for both copies at once, and a failed re-export never loses the model.
// A loaded entry kept, new or replacing, and a clear raise `changes`.
//
// A PICTURE IS A LOAD OF ITS OWN, chosen by the path's extension: decoded and
// uploaded by model_picture.h, its two parts on the store's one quad, which is
// made on the first picture or dot and freed at clear; releasing a picture entry
// never touches it. The dot is an entry and its held memory kept beside the
// table, so the table's count and indices never see it; the water's one part
// and record are kept beside it too.
//
// A LANDSCAPE IS READ HERE AND UPLOADED BY models_landscape.c, which holds
// every landscape call; the table they share is models_store.h's.
//
// CONSTRAINTS: _find is a scan over the entries by string compare, which at
// VOE_3D_MODELS entries is nothing; a hash of the path would lift it if the
// store ever grows by orders of magnitude. A full store refuses a new path as
// the limit it is, and _fail says so rather than keeping it.
#include "model_bake.h"
#include "model_picture.h"
#include "model_upload.h"
#include "models_store.h"

#include <3d/models.h>
#include <assets/landscape.h>
#include <assets/model.h>
#include <base/arena.h>
#include <base/assert.h>
#include <base/report.h>

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// An entry's arena starts this big and grows; a load's scratch arena too,
// since a decoded picture alone may be many megabytes.
#define ENTRY_BLOCK (64 * 1024)
#define SCRATCH_BLOCK (1024 * 1024)

// The store's limit, said as one (0445 point 5): the files it would hold with
// this one beside the most.
static void say_full(void)
{
	VOE_BASE_ERROR("3d",
		       "this would be %u files held at once, and the most the engine takes is %u, models, pictures, landscapes and materials together",
		       VOE_3D_MODELS + 1, VOE_3D_MODELS);
}

voe_3d_models *voe_3d_models_new(void)
{
	voe_3d_models *models = calloc(1, sizeof(*models));

	VOE_BASE_ASSERT(models != NULL, "out of memory making a model store");
	return models;
}

// Whether `part`'s `faded` is a twin record of its own and not its material's.
static bool has_twin(const voe_3d_model_part *part)
{
	return part->faded.index != part->material.shading.index ||
	       part->faded.generation != part->material.shading.generation;
}

// Everything `entry` put on the card and its memory, given back; a picture's
// parts are on the store's quad and a landscape's on its grid, neither the
// entry's to free, and a material's part has none. A loaded landscape gives
// back its heights texture. The levels go first: made ones walk the parts'
// vertices, and `render` asserts on a base destroyed under a sharer.
static void release(voe_render_device *device, const voe_3d_model_entry *entry,
		    const entry_held *held)
{
	voe_3d_models_levels_free(device, entry);
	for (uint32_t i = 0; i < entry->part_count; i++) {
		if (!entry->picture && entry->landscape == NULL &&
		    !entry->material)
			(void)voe_render_geometry_destroy(
				device, entry->parts[i].geometry);
		if (has_twin(&entry->parts[i]))
			(void)voe_render_shading_destroy(
				device, entry->parts[i].faded);
	}
	if (entry->loaded && entry->landscape != NULL)
		(void)voe_render_texture_destroy(device, held->heights);
	for (uint32_t i = 0; i < held->shading_count; i++)
		(void)voe_render_shading_destroy(device, held->shadings[i]);
	for (uint32_t i = 0; i < held->texture_count; i++)
		(void)voe_render_texture_destroy(device, held->textures[i]);
	voe_base_arena_destroy(held->memory);
}

void voe_3d_models_clear(voe_3d_models *models, voe_render_device *device)
{
	VOE_BASE_ASSERT(models != NULL, "clearing no store");
	VOE_BASE_ASSERT(device != NULL, "clearing a store with no device");

	for (uint32_t i = 0; i < models->count; i++)
		release(device, &models->entries[i], &models->held[i]);
	models->count = 0;
	models->changes++;
	if (models->has_dot)
		release(device, &models->dot, &models->dot_held);
	models->has_dot = false;
	if (models->has_water)
		(void)voe_render_shading_destroy(device,
						 models->water.material.shading);
	models->has_water = false;
	if (models->has_quad)
		(void)voe_render_geometry_destroy(device, models->quad);
	models->has_quad = false;
	if (models->has_grid)
		(void)voe_render_geometry_destroy(device, models->grid);
	models->has_grid = false;
}

void voe_3d_models_destroy(voe_3d_models *models)
{
	if (models == NULL)
		return;
	for (uint32_t i = 0; i < models->count; i++)
		voe_base_arena_destroy(models->held[i].memory);
	if (models->has_dot)
		voe_base_arena_destroy(models->dot_held.memory);
	free(models);
}

uint32_t voe_3d_models_index(const voe_3d_models *models, const char *path)
{
	for (uint32_t i = 0; i < models->count; i++) {
		if (strcmp(models->entries[i].path, path) == 0)
			return i;
	}
	return VOE_3D_MODELS;
}

bool voe_3d_models_room(const voe_3d_models *models, const char *path,
			voe_base_error *error)
{
	if (models->count < VOE_3D_MODELS ||
	    voe_3d_models_index(models, path) != VOE_3D_MODELS)
		return true;
	say_full();
	if (error != NULL)
		*error = VOE_BASE_ERROR_REFUSED;
	return false;
}

void voe_3d_models_entry_new(const char *path, uint64_t stamp,
			     voe_3d_model_entry *entry, entry_held *held)
{
	size_t length = strlen(path) + 1;
	char *copy;

	*held = (entry_held){ .memory = voe_base_arena_new(ENTRY_BLOCK) };
	copy = voe_base_arena_push(held->memory, length);
	memcpy(copy, path, length);
	*entry = (voe_3d_model_entry){ .path = copy,
				       .stamp = stamp,
				       .levels[0].bound = INFINITY,
				       .level_count = 1 };
}

// Puts a new entry into the store for a path it does not hold, or, for one it
// does, replaces the old entry when the new one loaded and keeps the old one at
// the new stamp when it did not. Whatever is not kept is given back. False,
// with the new entry given back, when the store is full. A loaded entry kept,
// new or replacing, counts as a change.
bool voe_3d_models_keep(voe_3d_models *models, voe_render_device *device,
			const voe_3d_model_entry *entry, const entry_held *held)
{
	uint32_t index = voe_3d_models_index(models, entry->path);
	voe_3d_model_entry kept = *entry;

	voe_3d_models_levels_mirror(&kept);
	if (index == VOE_3D_MODELS) {
		if (models->count == VOE_3D_MODELS) {
			say_full();
			release(device, entry, held);
			return false;
		}
		if (entry->loaded)
			models->changes++;
		models->entries[models->count] = kept;
		models->held[models->count++] = *held;
		return true;
	}
	if (entry->loaded) {
		voe_3d_model_entry old = models->entries[index];
		entry_held old_held = models->held[index];

		models->entries[index] = kept;
		models->held[index] = *held;
		models->changes++;
		release(device, &old, &old_held);
	} else {
		models->entries[index].stamp = entry->stamp;
		release(device, entry, held);
	}
	return true;
}

// `size` bytes of `from` copied into `memory`; NULL for none.
static void *copy_into(voe_base_arena *memory, const void *from, size_t size)
{
	void *to;

	if (size == 0)
		return NULL;
	to = voe_base_arena_push(memory, size);
	memcpy(to, from, size);
	return to;
}

// The prepared shape copied into the entry's arena, its three arrays with it,
// so the prepare's arena may go once the put returns.
static void copy_shape(voe_base_arena *memory,
		       const voe_3d_shape_geometry *shape,
		       voe_3d_shape_geometry *out)
{
	*out = (voe_3d_shape_geometry){
		.vertices = copy_into(memory, shape->vertices,
				      (size_t)shape->vertex_count *
					      sizeof(*shape->vertices)),
		.vertex_count = shape->vertex_count,
		.indices = copy_into(memory, shape->indices,
				     (size_t)shape->index_count *
					     sizeof(*shape->indices)),
		.index_count = shape->index_count,
		.edges = copy_into(memory, shape->edges,
				   (size_t)shape->edge_count *
					   sizeof(*shape->edges)),
		.edge_count = shape->edge_count,
	};
}

// `part`'s twin, its material with the alpha mode BLENDED, when it is not
// BLENDED already; false, with `error` REFUSED, when the device has no room.
bool voe_3d_models_twin(voe_render_device *device, voe_3d_model_part *part,
			voe_base_error *error)
{
	voe_3d_material twin = part->material;

	if (twin.alpha_mode == VOE_RENDER_ALPHA_BLENDED)
		return true;
	twin.alpha_mode = VOE_RENDER_ALPHA_BLENDED;
	if (!voe_3d_material_upload(device, &twin, error))
		return false;
	part->faded = twin.shading;
	return true;
}

// Every part a geometry and a twin; false at the first the device has no room
// for, with `made` saying how many parts have a geometry.
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
		entry->parts[p].faded = entry->parts[p].material.shading;
		entry->parts[p].source = part->material;
		*made = p + 1;
		if (!voe_3d_models_twin(device, &entry->parts[p], error))
			return false;
	}
	return true;
}

// What a failed load had put on the card, given back, the levels first as
// release() gives them.
static void give_back(voe_render_device *device,
		      const voe_3d_model_upload *upload,
		      const voe_3d_model_entry *entry, uint32_t geometries)
{
	voe_3d_models_levels_free(device, entry);
	for (uint32_t i = 0; i < geometries; i++) {
		(void)voe_render_geometry_destroy(device,
						  entry->parts[i].geometry);
		if (has_twin(&entry->parts[i]))
			(void)voe_render_shading_destroy(
				device, entry->parts[i].faded);
	}
	for (uint32_t i = 0; i < upload->shading_count; i++)
		(void)voe_render_shading_destroy(device, upload->shadings[i]);
	for (uint32_t i = 0; i < upload->texture_count; i++)
		(void)voe_render_texture_destroy(device, upload->textures[i]);
}

// The load's lists of what it made, copied into the entry's arena.
static void hold_upload(const voe_3d_model_upload *upload, entry_held *held)
{
	size_t textures = upload->texture_count * sizeof(*held->textures);
	size_t shadings = upload->shading_count * sizeof(*held->shadings);

	if (upload->texture_count > 0) {
		held->textures = voe_base_arena_push(held->memory, textures);
		memcpy(held->textures, upload->textures, textures);
	}
	if (upload->shading_count > 0) {
		held->shadings = voe_base_arena_push(held->memory, shadings);
		memcpy(held->shadings, upload->shadings, shadings);
	}
	held->texture_count = upload->texture_count;
	held->shading_count = upload->shading_count;
}

// Uploads a prepared `.glb` and its levels into `entry` and copies its shape;
// on failure what it made is given back.
static bool upload_glb(voe_render_device *device, voe_base_arena *scratch,
		       const voe_3d_model_prepared *prepared,
		       voe_3d_model_entry *entry, entry_held *held,
		       voe_base_error *error)
{
	voe_3d_model_upload upload = { 0 };
	uint32_t made = 0;
	bool loaded;

	loaded = voe_3d_model_upload_create(device, scratch, prepared->model,
					    &upload, error) &&
		 upload_parts(device, &prepared->bake, &upload, entry, &made,
			      error);
	if (loaded) {
		entry->centre = prepared->centre;
		entry->radius = prepared->radius;
	}
	loaded = loaded &&
		 voe_3d_models_levels_upload(device, prepared, entry, error);

	if (loaded) {
		copy_shape(held->memory, &prepared->shape, &entry->shape);
		hold_upload(&upload, held);
		entry->part_count = prepared->bake.part_count;
		entry->loaded = true;
	} else {
		give_back(device, &upload, entry, made);
		entry->level_count = 1;
	}
	return loaded;
}

bool voe_3d_models_put(voe_3d_models *models, voe_render_device *device,
		       const char *path, uint64_t stamp,
		       const voe_3d_model_prepared *prepared,
		       voe_base_error *error)
{
	voe_3d_model_entry entry;
	voe_base_arena *scratch;
	entry_held held;
	bool loaded;

	VOE_BASE_ASSERT(models != NULL, "putting a model into no store");
	VOE_BASE_ASSERT(device != NULL, "putting a model with no device");
	VOE_BASE_ASSERT(path != NULL && prepared != NULL,
			"putting no path or nothing prepared");
	VOE_BASE_ASSERT(prepared->model->image_count == 0 ||
				prepared->model->images != NULL,
			"putting a model whose pictures are not decoded");

	if (!voe_3d_models_room(models, path, error))
		return false;
	voe_3d_models_entry_new(path, stamp, &entry, &held);
	scratch = voe_base_arena_new(SCRATCH_BLOCK);
	loaded = upload_glb(device, scratch, prepared, &entry, &held, error);
	voe_base_arena_destroy(scratch);
	(void)voe_3d_models_keep(models, device, &entry, &held);
	return loaded;
}

// Reads and prepares a `.glb` on a scratch arena of its own, then puts it; a
// file that will not read or prepare is kept as failed at `stamp`.
static bool load_glb(voe_3d_models *models, voe_render_device *device,
		     const char *path, uint64_t stamp, const uint8_t *bytes,
		     size_t size, const uint8_t *levels, size_t levels_size,
		     voe_base_error *error)
{
	voe_base_arena *scratch = voe_base_arena_new(SCRATCH_BLOCK);
	const voe_3d_model_prepared *prepared = NULL;
	voe_assets_model model;
	bool loaded = false;

	if (voe_assets_model_read_glb(bytes, size, scratch, &model, error))
		prepared = voe_3d_model_prepare(scratch, &model, bytes, size,
						levels, levels_size, error);
	if (prepared != NULL)
		loaded = voe_3d_models_put(models, device, path, stamp,
					   prepared, error);
	else
		voe_3d_models_fail(models, path, stamp);
	voe_base_arena_destroy(scratch);
	return loaded;
}

// The store's quad, made on its first picture or dot.
static bool quad_ready(voe_3d_models *models, voe_render_device *device,
		       voe_base_error *error)
{
	if (models->has_quad)
		return true;
	models->has_quad =
		voe_3d_model_picture_quad(device, &models->quad, error);
	return models->has_quad;
}

// Uploads a decoded picture into `entry` as its two parts on the quad; on
// failure what it made is given back.
static bool upload_picture(voe_3d_models *models, voe_render_device *device,
			   voe_base_arena *scratch,
			   const voe_assets_image *image,
			   voe_3d_model_entry *entry, entry_held *held,
			   voe_base_error *error)
{
	voe_3d_model_upload upload = { 0 };

	entry->picture = true;
	if (!quad_ready(models, device, error) ||
	    !voe_3d_model_picture_upload(device, scratch, image, &upload,
					 error)) {
		give_back(device, &upload, entry, 0);
		return false;
	}
	for (uint32_t p = 0; p < 2; p++)
		entry->parts[p] = (voe_3d_model_part){
			.geometry = models->quad,
			.material = upload.materials[p],
			.faded = upload.materials[p].shading,
			.source = VOE_ASSETS_MODEL_NONE,
		};
	hold_upload(&upload, held);
	entry->part_count = 2;
	entry->loaded = true;
	return true;
}

// Whether `path` ends `.landscape`, compared without case.
static bool is_landscape(const char *path)
{
	static const char suffix[] = ".landscape";
	const size_t tail = sizeof(suffix) - 1;
	const size_t length = strlen(path);

	if (length < tail)
		return false;
	for (size_t i = 0; i < tail; i++) {
		if (tolower((unsigned char)path[length - tail + i]) != suffix[i])
			return false;
	}
	return true;
}

// Reads a `.landscape`'s text and loads it; text that is no landscape is kept
// as a failed entry, or keeps the old one, at `stamp`.
static bool load_landscape_text(voe_3d_models *models,
				voe_render_device *device, const char *path,
				uint64_t stamp, const uint8_t *bytes,
				size_t size, voe_base_error *error)
{
	voe_base_arena *scratch = voe_base_arena_new(SCRATCH_BLOCK);
	voe_assets_landscape landscape;
	bool loaded = voe_assets_landscape_read((const char *)bytes, size,
						scratch, &landscape, error);

	if (loaded) {
		loaded = voe_3d_models_load_landscape(models, device, path, stamp,
						      &landscape, error);
	} else {
		voe_3d_models_fail(models, path, stamp);
	}
	voe_base_arena_destroy(scratch);
	return loaded;
}

bool voe_3d_models_load(voe_3d_models *models, voe_render_device *device,
			const char *path, uint64_t stamp, const uint8_t *bytes,
			size_t size, voe_base_error *error)
{
	return voe_3d_models_load_levelled(models, device, path, stamp, bytes,
					   size, NULL, 0, error);
}

bool voe_3d_models_load_levelled(voe_3d_models *models,
				 voe_render_device *device, const char *path,
				 uint64_t stamp, const uint8_t *bytes,
				 size_t size, const uint8_t *levels,
				 size_t levels_size, voe_base_error *error)
{
	voe_3d_model_entry entry;
	voe_base_arena *scratch;
	voe_assets_image image;
	entry_held held;
	bool loaded;

	VOE_BASE_ASSERT(models != NULL, "loading a model into no store");
	VOE_BASE_ASSERT(device != NULL, "loading a model with no device");
	VOE_BASE_ASSERT(path != NULL, "loading a model with no path");

	if (!voe_3d_models_room(models, path, error))
		return false;
	if (is_landscape(path))
		return load_landscape_text(models, device, path, stamp, bytes,
					   size, error);

	if (!voe_3d_model_picture_is(path))
		return load_glb(models, device, path, stamp, bytes, size,
				levels, levels_size, error);

	voe_3d_models_entry_new(path, stamp, &entry, &held);
	scratch = voe_base_arena_new(SCRATCH_BLOCK);
	entry.picture = true;
	loaded = voe_3d_model_picture_decode(path, bytes, size, scratch, &image,
					     error) &&
		 upload_picture(models, device, scratch, &image, &entry, &held,
				error);
	voe_base_arena_destroy(scratch);
	(void)voe_3d_models_keep(models, device, &entry, &held);
	return loaded;
}

bool voe_3d_models_load_dot(voe_3d_models *models, voe_render_device *device,
			    voe_base_error *error)
{
	voe_assets_image image;
	voe_base_arena *scratch;
	bool loaded;

	VOE_BASE_ASSERT(models != NULL, "loading the dot into no store");
	VOE_BASE_ASSERT(device != NULL, "loading the dot with no device");

	if (models->has_dot)
		return true;
	voe_3d_models_entry_new("", 0, &models->dot, &models->dot_held);
	scratch = voe_base_arena_new(SCRATCH_BLOCK);
	image = voe_3d_model_picture_dot(scratch);
	loaded = upload_picture(models, device, scratch, &image, &models->dot,
				&models->dot_held, error);
	voe_base_arena_destroy(scratch);
	if (!loaded) {
		VOE_BASE_ERROR("3d", "could not load the soft dot");
		voe_base_arena_destroy(models->dot_held.memory);
	}
	voe_3d_models_levels_mirror(&models->dot);
	models->has_dot = loaded;
	return loaded;
}

bool voe_3d_models_load_water(voe_3d_models *models, voe_render_device *device,
			      voe_base_error *error)
{
	const voe_render_texture none = { .index = VOE_RENDER_NO_TEXTURE };
	voe_3d_material material = {
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.metallic = 0.0f,
		.roughness = 0.05f,
		.alpha_mode = VOE_RENDER_ALPHA_BLENDED,
		.alpha_cutoff = 0.5f,
		.base_colour_uv_scale = { 1.0f, 1.0f },
		.base_colour_texture = none,
		.metallic_roughness_texture = none,
		.normal_texture = none,
		.occlusion_texture = none,
		.emissive_texture = none,
	};
	voe_render_shading_values values = {
		.base_colour = material.base_colour,
		.metallic = material.metallic,
		.roughness = material.roughness,
		.alpha_mode = (uint32_t)material.alpha_mode,
		.alpha_cutoff = material.alpha_cutoff,
		.water = 1u,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};

	VOE_BASE_ASSERT(models != NULL, "loading the water into no store");
	VOE_BASE_ASSERT(device != NULL, "loading the water with no device");

	if (models->has_water)
		return true;
	// The record is made here and not by voe_3d_material_upload, because
	// `water` is a field of the record and not of the material.
	if (!quad_ready(models, device, error) ||
	    !voe_render_shading_create(device, values, &material.shading,
				       error)) {
		VOE_BASE_ERROR("3d", "could not load the water");
		return false;
	}
	models->water = (voe_3d_model_part){ .geometry = models->quad,
					     .material = material,
					     .faded = material.shading,
					     .source = VOE_ASSETS_MODEL_NONE };
	models->has_water = true;
	return true;
}

const voe_3d_model_part *voe_3d_models_water(const voe_3d_models *models)
{
	VOE_BASE_ASSERT(models != NULL, "reading the water of no store");

	return models->has_water ? &models->water : NULL;
}

void voe_3d_models_fail(voe_3d_models *models, const char *path,
			uint64_t stamp)
{
	voe_3d_model_entry entry;
	entry_held held;

	VOE_BASE_ASSERT(models != NULL, "failing a model in no store");
	VOE_BASE_ASSERT(path != NULL, "failing a model with no path");

	// A failed entry has nothing on the card, so keep never reaches a
	// device for it.
	voe_3d_models_entry_new(path, stamp, &entry, &held);
	(void)voe_3d_models_keep(models, NULL, &entry, &held);
}

void voe_3d_models_parts_set(voe_3d_models *models, const char *path,
			     const voe_assets_parts *parts)
{
	uint32_t index;
	voe_3d_model_entry *entry;

	VOE_BASE_ASSERT(models != NULL, "setting parts in no store");
	VOE_BASE_ASSERT(path != NULL, "setting parts with no path");

	index = voe_3d_models_index(models, path);
	if (index == VOE_3D_MODELS || !models->entries[index].loaded)
		return;
	entry = &models->entries[index];
	for (uint32_t p = 0; p < entry->part_count; p++) {
		voe_3d_model_part *part = &entry->parts[p];
		const bool named = parts != NULL && part->source < parts->count;

		VOE_BASE_ASSERT(!named || memchr(parts->materials[part->source],
						 '\0', VOE_ASSETS_MATERIAL_PATH),
				"a parts file's path with no end");
		snprintf(part->wears, sizeof(part->wears), "%s",
			 named ? parts->materials[part->source] : "");
	}
}

const voe_3d_model_entry *voe_3d_models_find(const voe_3d_models *models,
					     const char *path)
{
	uint32_t index;

	VOE_BASE_ASSERT(models != NULL, "finding a model in no store");
	VOE_BASE_ASSERT(path != NULL, "finding a model with no path");

	if (path[0] == '\0')
		return models->has_dot ? &models->dot : NULL;
	index = voe_3d_models_index(models, path);
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
