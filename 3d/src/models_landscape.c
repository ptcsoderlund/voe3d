// The model store's landscapes (3d/models.h, 0379 points 2, 4 and 6): a grid
// copied into its entry and uploaded as 16 chunk parts, a brush or a put
// marking chunks dirty, the frame drawing dirty chunks transient, the settle
// uploading them static again, the saved mark and the rename.
//
// Used by models.c's _load for a `.landscape` path, and by the editor: the
// brush and the frame while a stroke is held, the settle between frames, the
// put for an undo step, the rename for a moved file.
//
// Constraints: dirt is per chunk, so one stamp rebuilds whole chunks, at 512
// cells 129² vertices each, and a height on a chunk edge rebuilds two; a dirty
// rect per chunk would lift it. A frame and a settle walk every entry.
#include "models_store.h"

#include <3d/landscape.h>
#include <3d/models.h>
#include <assets/landscape.h>
#include <base/arena.h>
#include <base/assert.h>
#include <base/report.h>

#include <string.h>

#define CHUNKS (VOE_3D_LANDSCAPE_CHUNKS * VOE_3D_LANDSCAPE_CHUNKS)
// A chunk's mesh starts this big on the load's own scratch, and grows.
#define SCRATCH_BLOCK (1024 * 1024)

static_assert(CHUNKS <= VOE_3D_MODEL_PARTS, "a part per chunk");
static_assert(CHUNKS <= 16, "a dirty bit per chunk");

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

// Chunk `chunk` of `landscape` built on `scratch`, which is rewound, and
// uploaded static or, `transient`, into this frame.
static bool upload_chunk(voe_render_device *device,
			 const voe_assets_landscape *landscape, uint32_t chunk,
			 bool transient, voe_base_arena *scratch,
			 voe_render_geometry *out, voe_base_error *error)
{
	const struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	const voe_3d_landscape_mesh mesh =
		voe_3d_landscape_chunk(landscape, chunk, scratch);
	const bool made =
		transient ? voe_render_geometry_create_transient(
				    device, mesh.vertices, mesh.vertex_count,
				    mesh.indices, mesh.index_count, out, error) :
			    voe_render_geometry_create(
				    device, mesh.vertices, mesh.vertex_count,
				    mesh.indices, mesh.index_count, out, error);

	voe_base_arena_rewind(scratch, mark);
	return made;
}

// Every chunk a static geometry, kept in `held->statics`; false at the first
// the device refuses, `made` saying how many it made.
static bool upload_chunks(voe_render_device *device,
			  const voe_assets_landscape *landscape,
			  entry_held *held, uint32_t *made,
			  voe_base_error *error)
{
	voe_base_arena *scratch = voe_base_arena_new(SCRATCH_BLOCK);
	bool uploaded = true;

	for (uint32_t c = 0; uploaded && c < CHUNKS; c++) {
		uploaded = upload_chunk(device, landscape, c, false, scratch,
					&held->statics[c], error);
		*made = uploaded ? c + 1 : c;
	}
	voe_base_arena_destroy(scratch);
	return uploaded;
}

// Uploads `entry`'s grid as its parts; on failure gives back what it made.
static bool upload_landscape(voe_render_device *device,
			     voe_3d_model_entry *entry, entry_held *held,
			     voe_base_error *error)
{
	voe_3d_model_part part = { .material = ground() };
	uint32_t made = 0;
	const bool material =
		voe_3d_material_upload(device, &part.material, error);
	bool twin = false;

	part.faded = part.material.shading;
	twin = material && voe_3d_models_twin(device, &part, error);
	if (twin && upload_chunks(device, entry->landscape, held, &made, error)) {
		for (uint32_t c = 0; c < CHUNKS; c++) {
			entry->parts[c] = part;
			entry->parts[c].geometry = held->statics[c];
		}
		held->shadings = voe_base_arena_push(held->memory,
						     sizeof(*held->shadings));
		held->shadings[0] = part.material.shading;
		held->shading_count = 1;
		entry->part_count = CHUNKS;
		entry->loaded = true;
		return true;
	}
	for (uint32_t c = 0; c < made; c++)
		(void)voe_render_geometry_destroy(device, held->statics[c]);
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
				landscape->cells % VOE_3D_LANDSCAPE_CHUNKS == 0,
			"loading a landscape of whole chunks");

	if (!voe_3d_models_room(models, path, error))
		return false;
	voe_3d_models_entry_new(path, stamp, &entry, &held);
	entry.landscape = copy_grid(held.memory, landscape);
	loaded = upload_landscape(device, &entry, &held, error);
	if (!loaded) {
		VOE_BASE_ERROR("3d", "could not load the landscape %s", path);
		entry.landscape = NULL;
	}
	(void)voe_3d_models_keep(models, device, &entry, &held);
	return loaded;
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

// The chunks `heights` touches marked dirty, and the entry edited when any.
static void mark_dirty(voe_3d_models *models, uint32_t index,
		       voe_3d_landscape_rect heights)
{
	const voe_3d_landscape_rect chunks = voe_3d_landscape_chunks(
		models->entries[index].landscape, heights);

	for (uint32_t cz = chunks.z0; cz < chunks.z1; cz++) {
		for (uint32_t cx = chunks.x0; cx < chunks.x1; cx++)
			models->held[index].dirty |= (uint16_t)(
				1u << (cz * VOE_3D_LANDSCAPE_CHUNKS + cx));
	}
	if (chunks.x0 < chunks.x1 && chunks.z0 < chunks.z1)
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
	mark_dirty(models, index, rect);
	return rect;
}

void voe_3d_models_landscape_put(voe_3d_models *models, const char *path,
				 voe_3d_landscape_rect rect, const float *values)
{
	VOE_BASE_ASSERT(models != NULL && path != NULL,
			"putting heights needs a store and a path");
	const uint32_t index = landscape_index(models, path);

	if (index == VOE_3D_MODELS || rect.x0 >= rect.x1 || rect.z0 >= rect.z1)
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
	mark_dirty(models, index, rect);
}

void voe_3d_models_landscape_frame(voe_3d_models *models,
				   voe_render_device *device,
				   voe_base_arena *scratch)
{
	VOE_BASE_ASSERT(models != NULL && device != NULL,
			"a landscape frame needs a store and a device");
	VOE_BASE_ASSERT(scratch != NULL, "a landscape frame needs scratch");

	for (uint32_t i = 0; i < models->count; i++) {
		voe_3d_model_entry *entry = &models->entries[i];
		const entry_held *held = &models->held[i];

		for (uint32_t c = 0; held->dirty != 0 && c < CHUNKS; c++) {
			voe_base_error error = VOE_BASE_OK;
			voe_render_geometry drawn;

			if ((held->dirty & (1u << c)) == 0)
				continue;
			if (upload_chunk(device, entry->landscape, c, true,
					 scratch, &drawn, &error)) {
				entry->parts[c].geometry = drawn;
			} else {
				VOE_BASE_ERROR("3d",
					       "chunk %u of %s drawn as last settled",
					       c, entry->path);
				entry->parts[c].geometry = held->statics[c];
			}
		}
	}
}

// Chunk `c` of entry `i` uploaded static, its old static destroyed and its
// dirt cleared; false, the old kept and drawn, when the device has no room.
static bool settle_chunk(voe_3d_models *models, voe_render_device *device,
			 uint32_t i, uint32_t c, voe_base_arena *scratch,
			 voe_base_error *error)
{
	voe_3d_model_entry *entry = &models->entries[i];
	entry_held *held = &models->held[i];
	voe_render_geometry fresh;

	if (!upload_chunk(device, entry->landscape, c, false, scratch, &fresh,
			  error)) {
		VOE_BASE_ERROR("3d", "chunk %u of %s could not settle", c,
			       entry->path);
		entry->parts[c].geometry = held->statics[c];
		return false;
	}
	(void)voe_render_geometry_destroy(device, held->statics[c]);
	held->statics[c] = fresh;
	entry->parts[c].geometry = fresh;
	held->dirty &= (uint16_t)~(1u << c);
	return true;
}

bool voe_3d_models_landscape_settle(voe_3d_models *models,
				    voe_render_device *device,
				    voe_base_arena *scratch,
				    voe_base_error *error)
{
	VOE_BASE_ASSERT(models != NULL && device != NULL,
			"settling needs a store and a device");
	VOE_BASE_ASSERT(scratch != NULL, "settling needs scratch");

	for (uint32_t i = 0; i < models->count; i++) {
		for (uint32_t c = 0; models->held[i].dirty != 0 && c < CHUNKS;
		     c++) {
			if ((models->held[i].dirty & (1u << c)) != 0 &&
			    !settle_chunk(models, device, i, c, scratch, error))
				return false;
		}
	}
	return true;
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
