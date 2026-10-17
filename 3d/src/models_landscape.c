// The model store's landscapes (3d/models.h; 0379 points 2 and 6, 0396 points
// 3 and 5, 0402 points 1, 3 and 4, 0442 points 1 and 2): a grid copied into its
// entry with its layers, paint, kinds and copies, every copy placed, uploaded
// as a heights texture and two paint textures and built into a min/max
// pyramid, worn as one part on the store's shared grid; a brush or a put
// updating the pyramid, placing again the copies over it and growing the
// heights' dirty rect, or a paint brush or put growing the paint's; a layer
// set; the frame writing both kinds of dirt a shared budget at a time; the
// lookup the draw files ask; the saved mark and the rename, layer and kind
// paths following. The foliage calls are models_foliage.c's.
//
// Used by models.c's _load for a `.landscape` path, by the draw files through
// voe_3d_models_terrain_of, and by the editor: the brush and the frame while a
// stroke is held, the puts for an undo step, the layer set for a dropped
// material, the rename for a moved file.
//
// Constraints: an entry's dirt is one rect a kind, so two stamps far apart
// write all the points between them; a list of rects would lift it. The frame
// writes whole rows of the rect, the first entries first, so one big put can
// hold another landscape's dirt back a few frames. The frame, the rename and
// the lookup walk every entry. A layer or kind path that would not fit renamed
// is kept. A landscape holds room for every copy it may have, about 4 MB.
#include "models_store.h"

#include <3d/foliage.h>
#include <3d/landscape.h>
#include <3d/models.h>
#include <assets/landscape.h>
#include <base/arena.h>
#include <base/assert.h>
#include <base/report.h>

#include <string.h>

// The shared grid's quads a side.
#define GRID VOE_3D_LANDSCAPE_NODE_QUADS
#define GRID_BLOCK (128 * 1024)
// A point's paint bytes in each plane, and in both.
#define TEXEL 4
#define PAINT_BYTES (2 * TEXEL)

// 0379 point 2's ground until 085 paints it: lit, opaque, linear olive.
static voe_3d_material ground(void)
{
	const voe_render_texture none = { .index = VOE_RENDER_NO_TEXTURE };

	return (voe_3d_material){
		.base_colour = { 0.24f, 0.30f, 0.16f, 1.0f },
		.metallic = 0.0f,
		.roughness = 0.9f,
		.alpha_mode = VOE_RENDER_ALPHA_OPAQUE,
		.alpha_cutoff = 0.5f,
		.base_colour_uv_scale = { 1.0f, 1.0f },
		.base_colour_texture = none,
		.metallic_roughness_texture = none,
		.normal_texture = none,
		.occlusion_texture = none,
		.emissive_texture = none,
	};
}

// `from`'s grid copied onto `memory`, its layers and kinds carried and its
// paint owned, every point all layer 0 when `from` has none; its copies owned
// with room for the cap, so a scatter never grows them.
static voe_assets_landscape *copy_grid(voe_base_arena *memory,
				       const voe_assets_landscape *from)
{
	const size_t points = (size_t)(from->cells + 1) * (from->cells + 1);
	voe_assets_landscape *to = voe_base_arena_push(memory, sizeof(*to));

	*to = *from;
	to->copies = voe_base_arena_push(
		memory, VOE_ASSETS_LANDSCAPE_COPIES * sizeof(*to->copies));
	if (from->copy_count > 0)
		memcpy(to->copies, from->copies,
		       from->copy_count * sizeof(*to->copies));
	to->heights = voe_base_arena_push(memory, points * sizeof(float));
	memcpy(to->heights, from->heights, points * sizeof(float));
	to->paint = voe_base_arena_push(memory, points * PAINT_BYTES);
	if (from->paint != NULL) {
		memcpy(to->paint, from->paint, points * PAINT_BYTES);
	} else {
		memset(to->paint, 0, points * PAINT_BYTES);
		for (size_t i = 0; i < points; i++)
			to->paint[i * TEXEL] = 255;
	}
	return to;
}

// The two paint planes as DATA textures, FIELD-sampled, listed in `textures`
// so the entry's release frees them; on failure none is left made.
static bool upload_paint(voe_render_device *device,
			 const voe_assets_landscape *land, entry_held *held,
			 voe_base_error *error)
{
	const uint32_t n = land->cells + 1;
	const size_t plane = (size_t)n * n * TEXEL;

	for (uint32_t p = 0; p < 2; p++) {
		if (voe_render_texture_create(device, VOE_RENDER_TEXTURE_DATA,
					      VOE_RENDER_SAMPLING_FIELD, n, n,
					      land->paint + p * plane,
					      &held->paint[p], error))
			continue;
		if (p == 1)
			(void)voe_render_texture_destroy(device, held->paint[0]);
		return false;
	}
	held->textures = voe_base_arena_push(held->memory, sizeof(held->paint));
	memcpy(held->textures, held->paint, sizeof(held->paint));
	held->texture_count = 2;
	return true;
}

// The store's grid (models_store.h), made on the first landscape. Corner
// (i, j) then (i, j + 1) then (i + 1, j) turns counter-clockwise seen from +Y.
static bool grid_ready(voe_3d_models *models, voe_render_device *device,
		       voe_base_error *error)
{
	const uint32_t side = GRID + 1;
	voe_base_arena *arena;
	voe_render_vertex *vertices;
	uint32_t *indices, *out;

	if (models->has_grid)
		return true;
	arena = voe_base_arena_new(GRID_BLOCK);
	vertices = voe_base_arena_push(arena, sizeof(*vertices) * side * side);
	indices = voe_base_arena_push(arena, sizeof(*indices) * GRID * GRID * 6);
	for (uint32_t j = 0; j < side; j++) {
		for (uint32_t i = 0; i < side; i++) {
			const float x = (float)i / GRID, z = (float)j / GRID;

			vertices[j * side + i] = (voe_render_vertex){
				.position = { x, j == GRID ? 1.0f : 0.0f, z },
				.normal = { 0.0f, 1.0f, 0.0f },
				.uv = { x, z },
			};
		}
	}
	out = indices;
	for (uint32_t j = 0; j < GRID; j++) {
		for (uint32_t i = 0; i < GRID; i++) {
			const uint32_t a = j * side + i, b = a + side;

			*out++ = a, *out++ = b, *out++ = a + 1;
			*out++ = a + 1, *out++ = b, *out++ = b + 1;
		}
	}
	models->has_grid = voe_render_geometry_create(
		device, vertices, side * side, indices, GRID * GRID * 6,
		&models->grid, error);
	voe_base_arena_destroy(arena);
	return models->has_grid;
}

// `entry`'s heights as its texture and pyramid, its paint as two textures, its
// one part the grid; on failure gives back what it made, the grid kept.
static bool upload_landscape(voe_3d_models *models, voe_render_device *device,
			     voe_3d_model_entry *entry, entry_held *held,
			     voe_base_error *error)
{
	const uint32_t n = entry->landscape->cells + 1;
	voe_3d_model_part part = { .material = ground(),
				   .source = VOE_ASSETS_MODEL_NONE };
	const bool material = grid_ready(models, device, error) &&
			      voe_3d_material_upload(device, &part.material,
						     error);
	bool twin = false, heights = false;

	part.faded = part.material.shading;
	twin = material && voe_3d_models_twin(device, &part, error);
	heights = twin && voe_render_texture_create_heights(
				  device, n, n, entry->landscape->heights,
				  &held->heights, error);
	if (heights && upload_paint(device, entry->landscape, held, error)) {
		part.geometry = models->grid;
		entry->parts[0] = part;
		entry->part_count = 1;
		entry->loaded = true;
		held->shadings = voe_base_arena_push(held->memory,
						     sizeof(*held->shadings));
		held->shadings[0] = part.material.shading;
		held->shading_count = 1;
		held->lod = voe_3d_landscape_lod_build(entry->landscape,
						       held->memory);
		return true;
	}
	if (heights)
		(void)voe_render_texture_destroy(device, held->heights);
	if (twin)
		(void)voe_render_shading_destroy(device, part.faded);
	if (material)
		(void)voe_render_shading_destroy(device, part.material.shading);
	return false;
}

bool voe_3d_models_load_landscape(voe_3d_models *models,
				  voe_render_device *device, const char *path,
				  uint64_t stamp,
				  const voe_assets_landscape *landscape,
				  voe_base_error *error)
{
	voe_3d_model_entry entry;
	entry_held held;
	bool loaded;

	VOE_BASE_ASSERT(models != NULL && device != NULL && path != NULL,
			"loading a landscape needs a store, a device and a path");
	VOE_BASE_ASSERT(landscape != NULL && landscape->heights != NULL &&
				landscape->cells >= 1 &&
				landscape->cells <= VOE_ASSETS_LANDSCAPE_CELLS_MAX,
			"loading a landscape of 1 to the most cells");
	VOE_BASE_ASSERT(landscape->layer_count <= VOE_ASSETS_LANDSCAPE_LAYERS,
			"loading a landscape of at most eight layers");
	VOE_BASE_ASSERT(landscape->kind_count <= VOE_ASSETS_LANDSCAPE_KINDS &&
				landscape->copy_count <=
					VOE_ASSETS_LANDSCAPE_COPIES &&
				(landscape->copies != NULL ||
				 landscape->copy_count == 0),
			"loading a landscape of at most the kinds and copies");
	const uint32_t n = landscape->cells + 1;

	if (!voe_3d_models_room(models, path, error))
		return false;
	voe_3d_models_entry_new(path, stamp, &entry, &held);
	entry.landscape = copy_grid(held.memory, landscape);
	held.placed = voe_base_arena_push(
		held.memory, VOE_ASSETS_LANDSCAPE_COPIES * sizeof(*held.placed));
	voe_3d_foliage_replace(entry.landscape, held.placed,
			       (voe_3d_landscape_rect){ 0, 0, n, n });
	loaded = upload_landscape(models, device, &entry, &held, error);
	if (!loaded) {
		VOE_BASE_ERROR("3d", "could not load the landscape %s", path);
		entry.landscape = NULL;
	}
	(void)voe_3d_models_keep(models, device, &entry, &held);
	return loaded;
}

bool voe_3d_models_terrain_of(const voe_3d_models *models,
			      const voe_3d_model_entry *entry,
			      voe_3d_models_terrain *out)
{
	VOE_BASE_ASSERT(models != NULL && entry != NULL,
			"a terrain needs a store and an entry");
	VOE_BASE_ASSERT(out != NULL, "somewhere to put the terrain");

	for (uint32_t i = 0; i < models->count; i++) {
		if (&models->entries[i] != entry)
			continue;
		if (!entry->loaded || entry->landscape == NULL)
			return false;
		const entry_held *held = &models->held[i];

		*out = (voe_3d_models_terrain){
			.heights = held->heights,
			.lod = &held->lod,
			.grid = models->grid,
			.paint = { held->paint[0], held->paint[1] },
		};
		return true;
	}
	return false;
}

uint32_t voe_3d_models_landscape_index(const voe_3d_models *models,
				       const char *path)
{
	VOE_BASE_ASSERT(models != NULL && path != NULL,
			"finding a landscape needs a store and a path");
	const uint32_t index = voe_3d_models_index(models, path);

	if (index == VOE_3D_MODELS || !models->entries[index].loaded ||
	    models->entries[index].landscape == NULL)
		return VOE_3D_MODELS;
	return index;
}

static bool is_empty(voe_3d_landscape_rect rect)
{
	return rect.x0 >= rect.x1 || rect.z0 >= rect.z1;
}

// `dirty` grown to hold the non-empty `changed`.
static void grow(voe_3d_landscape_rect *dirty, voe_3d_landscape_rect changed)
{
	if (is_empty(*dirty)) {
		*dirty = changed;
		return;
	}
	dirty->x0 = changed.x0 < dirty->x0 ? changed.x0 : dirty->x0;
	dirty->z0 = changed.z0 < dirty->z0 ? changed.z0 : dirty->z0;
	dirty->x1 = changed.x1 > dirty->x1 ? changed.x1 : dirty->x1;
	dirty->z1 = changed.z1 > dirty->z1 ? changed.z1 : dirty->z1;
}

// The heights `changed` refreshed in the pyramid, the copies over them placed
// again, the rect added to the dirty rect, and the entry edited; nothing for an
// empty rect.
static void grow_dirt(voe_3d_models *models, uint32_t index,
		      voe_3d_landscape_rect changed)
{
	entry_held *held = &models->held[index];

	if (is_empty(changed))
		return;
	voe_3d_landscape_lod_update(&held->lod,
				    models->entries[index].landscape, changed);
	voe_3d_foliage_replace(models->entries[index].landscape, held->placed,
			       changed);
	grow(&held->dirty, changed);
	models->entries[index].edited = true;
}

// The paint `changed` added to the paint's dirty rect and the entry edited;
// nothing for an empty rect.
static void grow_paint(voe_3d_models *models, uint32_t index,
		       voe_3d_landscape_rect changed)
{
	if (is_empty(changed))
		return;
	grow(&models->held[index].paint_dirty, changed);
	models->entries[index].edited = true;
}

voe_3d_landscape_rect
voe_3d_models_landscape_brush(voe_3d_models *models, const char *path,
			      const voe_3d_brush *brush, float x, float z,
			      float seconds, voe_base_arena *scratch)
{
	VOE_BASE_ASSERT(models != NULL && path != NULL,
			"brushing needs a store and a path");
	VOE_BASE_ASSERT(brush != NULL && scratch != NULL,
			"brushing needs a brush and scratch");
	const uint32_t index = voe_3d_models_landscape_index(models, path);
	voe_3d_landscape_rect rect;

	if (index == VOE_3D_MODELS)
		return (voe_3d_landscape_rect){ 0 };
	rect = voe_3d_landscape_brush(models->entries[index].landscape, brush,
				      x, z, seconds, scratch);
	if (brush->kind == VOE_3D_BRUSH_PAINT)
		grow_paint(models, index, rect);
	else
		grow_dirt(models, index, rect);
	return rect;
}

void voe_3d_models_landscape_put(voe_3d_models *models, const char *path,
				 voe_3d_landscape_rect rect, const float *values)
{
	VOE_BASE_ASSERT(models != NULL && path != NULL,
			"putting heights needs a store and a path");
	const uint32_t index = voe_3d_models_landscape_index(models, path);

	if (index == VOE_3D_MODELS || is_empty(rect))
		return;
	voe_assets_landscape *land = models->entries[index].landscape;
	const uint32_t n = land->cells + 1;
	const uint32_t width = rect.x1 - rect.x0;

	VOE_BASE_ASSERT(values != NULL, "putting no heights");
	VOE_BASE_ASSERT(rect.x1 <= n && rect.z1 <= n, "a rect on the grid");
	for (uint32_t r = rect.z0; r < rect.z1; r++)
		memcpy(&land->heights[(size_t)r * n + rect.x0],
		       &values[(size_t)(r - rect.z0) * width],
		       width * sizeof(float));
	grow_dirt(models, index, rect);
}

void voe_3d_models_landscape_put_paint(voe_3d_models *models, const char *path,
				       voe_3d_landscape_rect rect,
				       const uint8_t *values)
{
	VOE_BASE_ASSERT(models != NULL && path != NULL,
			"putting paint needs a store and a path");
	const uint32_t index = voe_3d_models_landscape_index(models, path);

	if (index == VOE_3D_MODELS || is_empty(rect))
		return;
	voe_assets_landscape *land = models->entries[index].landscape;
	const uint32_t n = land->cells + 1;
	const size_t plane = (size_t)n * n * TEXEL;
	const size_t row = (size_t)(rect.x1 - rect.x0) * TEXEL;
	const size_t given = row * (rect.z1 - rect.z0);

	VOE_BASE_ASSERT(values != NULL, "putting no paint");
	VOE_BASE_ASSERT(rect.x1 <= n && rect.z1 <= n, "a rect on the grid");
	for (uint32_t p = 0; p < 2; p++) {
		for (uint32_t r = rect.z0; r < rect.z1; r++)
			memcpy(&land->paint[p * plane +
					    ((size_t)r * n + rect.x0) * TEXEL],
			       &values[p * given + (r - rect.z0) * row], row);
	}
	grow_paint(models, index, rect);
}

void voe_3d_models_landscape_layer_set(voe_3d_models *models, const char *path,
				       uint32_t index, const char *material)
{
	VOE_BASE_ASSERT(models != NULL && path != NULL,
			"setting a layer needs a store and a path");
	VOE_BASE_ASSERT(material != NULL &&
				strlen(material) < VOE_ASSETS_MATERIAL_PATH,
			"a layer's material path that fits");
	const uint32_t at = voe_3d_models_landscape_index(models, path);

	if (at == VOE_3D_MODELS)
		return;
	voe_assets_landscape *land = models->entries[at].landscape;

	VOE_BASE_ASSERT(index <= land->layer_count &&
				index < VOE_ASSETS_LANDSCAPE_LAYERS,
			"a held layer or the next of eight");
	memcpy(land->layers[index], material, strlen(material) + 1);
	if (index == land->layer_count)
		land->layer_count++;
	models->entries[at].edited = true;
}

// Entry `i`'s dirty rows written into its texture while `budget` texels last,
// gathered in the store's `written`; the budget left, nought after a refusal.
static uint32_t write_dirt(voe_3d_models *models, voe_render_device *device,
			   uint32_t i, uint32_t budget)
{
	entry_held *held = &models->held[i];
	const voe_3d_landscape_rect dirty = held->dirty;
	const voe_3d_model_entry *entry = &models->entries[i];
	voe_base_error error = VOE_BASE_OK;

	if (is_empty(dirty))
		return budget;
	VOE_BASE_ASSERT(entry->loaded && entry->landscape != NULL,
			"dirt only on a loaded landscape");
	const uint32_t n = entry->landscape->cells + 1;
	const uint32_t width = dirty.x1 - dirty.x0;
	const uint32_t most = budget / width;
	const uint32_t rows =
		dirty.z1 - dirty.z0 < most ? dirty.z1 - dirty.z0 : most;

	if (rows == 0)
		return budget;
	for (uint32_t r = 0; r < rows; r++)
		memcpy(&models->written[(size_t)r * width],
		       &entry->landscape->heights[(size_t)(dirty.z0 + r) * n +
						  dirty.x0],
		       width * sizeof(float));
	if (!voe_render_texture_write_heights(device, held->heights, dirty.x0,
					      dirty.z0, width, rows,
					      models->written, &error)) {
		VOE_BASE_ERROR("3d", "the heights of %s wait for a later frame",
			       entry->path);
		return 0;
	}
	held->dirty.z0 += rows;
	if (is_empty(held->dirty))
		held->dirty = (voe_3d_landscape_rect){ 0 };
	return budget - rows * width;
}

// Entry `i`'s dirty paint rows written into both its textures while `budget`
// texels last, a row costing its width once a plane, gathered as bytes in the
// store's `written`; the budget left, nought after a refusal.
static uint32_t write_paint(voe_3d_models *models, voe_render_device *device,
			    uint32_t i, uint32_t budget)
{
	entry_held *held = &models->held[i];
	const voe_3d_landscape_rect dirty = held->paint_dirty;
	const voe_3d_model_entry *entry = &models->entries[i];
	uint8_t *written = (uint8_t *)models->written;
	voe_base_error error = VOE_BASE_OK;

	if (is_empty(dirty))
		return budget;
	VOE_BASE_ASSERT(entry->loaded && entry->landscape != NULL,
			"paint dirt only on a loaded landscape");
	const uint32_t n = entry->landscape->cells + 1;
	const size_t plane = (size_t)n * n * TEXEL;
	const uint32_t width = dirty.x1 - dirty.x0;
	const uint32_t most = budget / (2 * width);
	const uint32_t rows =
		dirty.z1 - dirty.z0 < most ? dirty.z1 - dirty.z0 : most;

	if (rows == 0)
		return budget;
	for (uint32_t p = 0; p < 2; p++) {
		for (uint32_t r = 0; r < rows; r++)
			memcpy(&written[(size_t)r * width * TEXEL],
			       &entry->landscape->paint[p * plane +
							((size_t)(dirty.z0 + r) * n +
							 dirty.x0) * TEXEL],
			       (size_t)width * TEXEL);
		if (!voe_render_texture_write_pixels(device, held->paint[p],
						     dirty.x0, dirty.z0, width,
						     rows, written, &error)) {
			VOE_BASE_ERROR("3d", "the paint of %s waits for a later frame",
				       entry->path);
			return 0;
		}
	}
	held->paint_dirty.z0 += rows;
	if (is_empty(held->paint_dirty))
		held->paint_dirty = (voe_3d_landscape_rect){ 0 };
	return budget - 2 * rows * width;
}

void voe_3d_models_landscape_frame(voe_3d_models *models,
				   voe_render_device *device)
{
	VOE_BASE_ASSERT(models != NULL, "a landscape frame needs a store");
	VOE_BASE_ASSERT(device != NULL, "a landscape frame needs a device");
	uint32_t budget = VOE_3D_LANDSCAPE_WRITE_TEXELS;

	for (uint32_t i = 0; budget > 0 && i < models->count; i++) {
		budget = write_dirt(models, device, i, budget);
		budget = write_paint(models, device, i, budget);
	}
}

void voe_3d_models_landscape_saved(voe_3d_models *models, const char *path)
{
	VOE_BASE_ASSERT(models != NULL, "marking saved in no store");
	VOE_BASE_ASSERT(path != NULL, "marking saved no path");
	const uint32_t index = voe_3d_models_index(models, path);

	if (index != VOE_3D_MODELS)
		models->entries[index].edited = false;
}

// What follows `from` in `path` when `path` is `from` or under `from/`; NULL
// for any other path.
static const char *tail_after(const char *path, const char *from,
			      size_t from_length)
{
	if (strncmp(path, from, from_length) != 0 ||
	    (path[from_length] != '\0' && path[from_length] != '/'))
		return NULL;
	return path + from_length;
}

// `path`, of `room` bytes, takes `to` in place of `from` when under it; one
// that would not fit is kept and said on stderr.
static void rename_path(char *path, size_t room, const char *from,
			const char *to)
{
	VOE_BASE_ASSERT(path != NULL && room > 0, "a path and its room");
	VOE_BASE_ASSERT(from != NULL && to != NULL, "a rename's two paths");
	const size_t to_length = strlen(to);
	const char *rest = tail_after(path, from, strlen(from));

	if (rest == NULL)
		return;
	const size_t rest_length = strlen(rest) + 1;

	if (to_length + rest_length > room) {
		VOE_BASE_ERROR("3d", "the path %s is too long renamed; kept",
			       path);
		return;
	}
	memmove(path + to_length, rest, rest_length);
	memcpy(path, to, to_length);
}

// `land`'s layer and kind paths under `from` take `to` in its place.
static void rename_paths(voe_assets_landscape *land, const char *from,
			 const char *to)
{
	VOE_BASE_ASSERT(land->layer_count <= VOE_ASSETS_LANDSCAPE_LAYERS,
			"at most eight layers");
	VOE_BASE_ASSERT(land->kind_count <= VOE_ASSETS_LANDSCAPE_KINDS,
			"at most sixteen kinds");
	for (uint32_t k = 0; k < land->layer_count; k++)
		rename_path(land->layers[k], VOE_ASSETS_MATERIAL_PATH, from,
			    to);
	for (uint32_t k = 0; k < land->kind_count; k++)
		rename_path(land->kinds[k], VOE_ASSETS_LANDSCAPE_KIND_PATH, from,
			    to);
}

void voe_3d_models_rename(voe_3d_models *models, const char *from,
			  const char *to)
{
	VOE_BASE_ASSERT(models != NULL && to != NULL, "renaming needs a store");
	VOE_BASE_ASSERT(from != NULL && from[0] != '\0', "renaming no path");
	const size_t from_length = strlen(from);
	const size_t to_length = strlen(to);

	for (uint32_t i = 0; i < models->count; i++) {
		if (models->entries[i].landscape != NULL)
			rename_paths(models->entries[i].landscape, from, to);
		const char *rest =
			tail_after(models->entries[i].path, from, from_length);

		if (rest == NULL)
			continue;
		const size_t rest_length = strlen(rest) + 1;
		char *renamed = voe_base_arena_push(models->held[i].memory,
						    to_length + rest_length);

		memcpy(renamed, to, to_length);
		memcpy(renamed + to_length, rest, rest_length);
		models->entries[i].path = renamed;
	}
}
