// The model store's landscapes (3d/models.h; 0379 points 2 and 6, 0396 points
// 3 and 5): a grid copied into its entry, uploaded as a heights texture and
// built into a min/max pyramid, worn as one part on the store's shared grid; a
// brush or a put updating the pyramid and growing the dirty rect; the frame
// writing the dirt into the texture a budget at a time; the lookup the draw
// files ask; the saved mark and the rename.
//
// Used by models.c's _load for a `.landscape` path, by the draw files through
// voe_3d_models_terrain_of, and by the editor: the brush and the frame while a
// stroke is held, the put for an undo step, the rename for a moved file.
//
// Constraints: an entry's dirt is one rect, so two stamps far apart write all
// the heights between them; a list of rects would lift it. The frame writes
// whole rows of the rect, the first entries first, so one big put can hold
// another landscape's dirt back a few frames. The frame, the rename and the
// lookup walk every entry.
#include "models_store.h"

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

// `from`'s grid copied onto `memory`.
static voe_assets_landscape *copy_grid(voe_base_arena *memory,
				       const voe_assets_landscape *from)
{
	const size_t bytes =
		(size_t)(from->cells + 1) * (from->cells + 1) * sizeof(float);
	voe_assets_landscape *to = voe_base_arena_push(memory, sizeof(*to));

	*to = *from;
	to->heights = voe_base_arena_push(memory, bytes);
	memcpy(to->heights, from->heights, bytes);
	return to;
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

// `entry`'s heights as its texture and pyramid, its one part the grid; on
// failure gives back what it made, the grid kept for the store.
static bool upload_landscape(voe_3d_models *models, voe_render_device *device,
			     voe_3d_model_entry *entry, entry_held *held,
			     voe_base_error *error)
{
	const uint32_t n = entry->landscape->cells + 1;
	voe_3d_model_part part = { .material = ground() };
	const bool material = grid_ready(models, device, error) &&
			      voe_3d_material_upload(device, &part.material,
						     error);
	bool twin = false;

	part.faded = part.material.shading;
	twin = material && voe_3d_models_twin(device, &part, error);
	if (twin && voe_render_texture_create_heights(device, n, n,
						      entry->landscape->heights,
						      &held->heights, error)) {
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

	if (!voe_3d_models_room(models, path, error))
		return false;
	voe_3d_models_entry_new(path, stamp, &entry, &held);
	entry.landscape = copy_grid(held.memory, landscape);
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
		*out = (voe_3d_models_terrain){ .heights = models->held[i].heights,
						.lod = &models->held[i].lod,
						.grid = models->grid };
		return true;
	}
	return false;
}

// `path`'s index when it is a loaded landscape, else VOE_3D_MODELS.
static uint32_t landscape_index(const voe_3d_models *models, const char *path)
{
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

// The heights `changed` refreshed in the pyramid, added to the dirty rect, and
// the entry edited; nothing for an empty rect.
static void grow_dirt(voe_3d_models *models, uint32_t index,
		      voe_3d_landscape_rect changed)
{
	entry_held *held = &models->held[index];
	voe_3d_landscape_rect *dirty = &held->dirty;

	if (is_empty(changed))
		return;
	voe_3d_landscape_lod_update(&held->lod,
				    models->entries[index].landscape, changed);
	if (is_empty(*dirty)) {
		*dirty = changed;
	} else {
		dirty->x0 = changed.x0 < dirty->x0 ? changed.x0 : dirty->x0;
		dirty->z0 = changed.z0 < dirty->z0 ? changed.z0 : dirty->z0;
		dirty->x1 = changed.x1 > dirty->x1 ? changed.x1 : dirty->x1;
		dirty->z1 = changed.z1 > dirty->z1 ? changed.z1 : dirty->z1;
	}
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
	const uint32_t index = landscape_index(models, path);
	voe_3d_landscape_rect rect;

	if (index == VOE_3D_MODELS)
		return (voe_3d_landscape_rect){ 0 };
	rect = voe_3d_landscape_brush(models->entries[index].landscape, brush,
				      x, z, seconds, scratch);
	grow_dirt(models, index, rect);
	return rect;
}

void voe_3d_models_landscape_put(voe_3d_models *models, const char *path,
				 voe_3d_landscape_rect rect, const float *values)
{
	VOE_BASE_ASSERT(models != NULL && path != NULL,
			"putting heights needs a store and a path");
	const uint32_t index = landscape_index(models, path);

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

void voe_3d_models_landscape_frame(voe_3d_models *models,
				   voe_render_device *device)
{
	VOE_BASE_ASSERT(models != NULL, "a landscape frame needs a store");
	VOE_BASE_ASSERT(device != NULL, "a landscape frame needs a device");
	uint32_t budget = VOE_3D_LANDSCAPE_WRITE_TEXELS;

	for (uint32_t i = 0; budget > 0 && i < models->count; i++)
		budget = write_dirt(models, device, i, budget);
}

void voe_3d_models_landscape_saved(voe_3d_models *models, const char *path)
{
	VOE_BASE_ASSERT(models != NULL, "marking saved in no store");
	VOE_BASE_ASSERT(path != NULL, "marking saved no path");
	const uint32_t index = voe_3d_models_index(models, path);

	if (index != VOE_3D_MODELS)
		models->entries[index].edited = false;
}

void voe_3d_models_rename(voe_3d_models *models, const char *from,
			  const char *to)
{
	VOE_BASE_ASSERT(models != NULL && to != NULL, "renaming needs a store");
	VOE_BASE_ASSERT(from != NULL && from[0] != '\0', "renaming no path");
	const size_t from_length = strlen(from);
	const size_t to_length = strlen(to);

	for (uint32_t i = 0; i < models->count; i++) {
		const char *path = models->entries[i].path;

		if (strncmp(path, from, from_length) != 0 ||
		    (path[from_length] != '\0' && path[from_length] != '/'))
			continue;
		const char *rest = path + from_length;
		const size_t rest_length = strlen(rest) + 1;
		char *renamed = voe_base_arena_push(models->held[i].memory,
						    to_length + rest_length);

		memcpy(renamed, to, to_length);
		memcpy(renamed + to_length, rest, rest_length);
		models->entries[i].path = renamed;
	}
}
