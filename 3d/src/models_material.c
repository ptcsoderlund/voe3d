// The model store's materials (3d/models.h; 0399 points 2, 4 and 5): a
// `.material`'s values and its three maps uploaded as one part with no
// geometry, its record and its blended twin; a set taking new values and
// marking the entry; the frame rewriting every marked entry's two records.
//
// Used by the editor and the game: the load when a `.material` is read or a
// map of it changes, the set while a field is dragged, the frame once a frame
// before its first pass.
//
// Constraints: a map's picture is decoded by its path's extension, as a
// picture entry's is, so a map path that is no `.png`, `.jpg` or `.jpeg` fails
// MALFORMED. Each material uploads its own maps; a map shared by two materials
// costs two textures, which a store of pictures by path would lift. The set
// and the frame walk every entry.
#include "model_picture.h"
#include "models_store.h"

#include <3d/material_component.h>
#include <3d/models.h>
#include <assets/image.h>
#include <assets/material.h>
#include <base/arena.h>
#include <base/assert.h>
#include <base/report.h>

#include <string.h>

// A material's maps a load uploads at most.
#define MAPS 3
#define SCRATCH_BLOCK (1024 * 1024)

// `from`'s values on a zeroed material: opaque, the whole texture, no maps.
static void take_values(const voe_assets_material_file *from, voe_3d_material *to)
{
	to->base_colour = (voe_math_float4){ from->colour[0], from->colour[1],
					     from->colour[2], 1.0f };
	to->metallic = from->metal;
	to->roughness = from->roughness;
	to->uv_repeat = from->repeat;
	to->unlit = from->shader == VOE_ASSETS_MATERIAL_UNLIT;
}

static voe_3d_material material_of(const voe_assets_material_file *from)
{
	const voe_render_texture none = { .index = VOE_RENDER_NO_TEXTURE };
	voe_3d_material material = {
		.alpha_mode = VOE_RENDER_ALPHA_OPAQUE,
		.alpha_cutoff = 0.5f,
		.base_colour_uv_scale = { 1.0f, 1.0f },
		.base_colour_texture = none,
		.metallic_roughness_texture = none,
		.normal_texture = none,
		.occlusion_texture = none,
		.emissive_texture = none,
	};

	take_values(from, &material);
	return material;
}

// The textures one load made, so a failure gives them back.
typedef struct {
	voe_render_texture made[MAPS];
	uint32_t count;
} maps_made;

// `map`'s picture, named `path`, uploaded as `kind` into `slot`; nothing for a
// map of no bytes. MALFORMED when it will not decode.
static bool upload_map(voe_render_device *device, voe_base_arena *scratch,
		       const char *path, voe_3d_material_map map,
		       voe_render_texture_kind kind, maps_made *made,
		       voe_render_texture *slot, voe_base_error *error)
{
	voe_assets_image image;

	if (map.size == 0)
		return true;
	VOE_BASE_ASSERT(map.bytes != NULL, "a map of bytes at no address");
	if (!voe_3d_model_picture_is(path) ||
	    !voe_3d_model_picture_decode(path, map.bytes, map.size, scratch,
					 &image, error)) {
		VOE_BASE_ERROR("3d", "the map %s is no picture", path);
		if (error != NULL)
			*error = VOE_BASE_ERROR_MALFORMED;
		return false;
	}
	if (!voe_render_texture_create(device, kind, VOE_RENDER_SAMPLING_SMOOTH,
				       image.width, image.height, image.pixels,
				       slot, error))
		return false;
	made->made[made->count++] = *slot;
	return true;
}

// The maps, the record and its twin into `entry`; on failure what was made is
// given back.
static bool upload_material(voe_render_device *device, voe_base_arena *scratch,
			    const voe_assets_material_file *values,
			    voe_3d_material_maps maps, voe_3d_model_entry *entry,
			    entry_held *held, voe_base_error *error)
{
	voe_3d_model_part part = { .material = material_of(values) };
	voe_3d_material *material = &part.material;
	maps_made made = { 0 };
	bool record = false;

	const bool uploaded =
		upload_map(device, scratch, values->colour_map, maps.colour,
			   VOE_RENDER_TEXTURE_COLOUR, &made,
			   &material->base_colour_texture, error) &&
		upload_map(device, scratch, values->normal_map, maps.normal,
			   VOE_RENDER_TEXTURE_DATA, &made,
			   &material->normal_texture, error) &&
		upload_map(device, scratch, values->roughness_map,
			   maps.roughness, VOE_RENDER_TEXTURE_DATA, &made,
			   &material->metallic_roughness_texture, error) &&
		(record = voe_3d_material_upload(device, material, error));

	part.faded = material->shading;
	if (uploaded && voe_3d_models_twin(device, &part, error)) {
		held->textures = voe_base_arena_push(
			held->memory, sizeof(*held->textures) * MAPS);
		memcpy(held->textures, made.made,
		       sizeof(*held->textures) * made.count);
		held->texture_count = made.count;
		held->shadings = voe_base_arena_push(held->memory,
						     sizeof(*held->shadings));
		held->shadings[0] = material->shading;
		held->shading_count = 1;
		entry->parts[0] = part;
		entry->part_count = 1;
		entry->loaded = true;
		entry->material = true;
		return true;
	}
	if (record)
		(void)voe_render_shading_destroy(device, material->shading);
	for (uint32_t i = 0; i < made.count; i++)
		(void)voe_render_texture_destroy(device, made.made[i]);
	return false;
}

bool voe_3d_models_load_material(voe_3d_models *models,
				 voe_render_device *device, const char *path,
				 uint64_t stamp,
				 const voe_assets_material_file *material,
				 voe_3d_material_maps maps,
				 voe_base_error *error)
{
	voe_3d_model_entry entry;
	voe_base_arena *scratch;
	entry_held held;
	bool loaded;

	VOE_BASE_ASSERT(models != NULL && device != NULL && path != NULL,
			"loading a material needs a store, a device and a path");
	VOE_BASE_ASSERT(material != NULL, "loading no material");

	if (!voe_3d_models_room(models, path, error))
		return false;
	voe_3d_models_entry_new(path, stamp, &entry, &held);
	scratch = voe_base_arena_new(SCRATCH_BLOCK);
	loaded = upload_material(device, scratch, material, maps, &entry,
				 &held, error);
	voe_base_arena_destroy(scratch);
	if (!loaded)
		VOE_BASE_ERROR("3d", "could not load the material %s", path);
	(void)voe_3d_models_keep(models, device, &entry, &held);
	return loaded;
}

void voe_3d_models_material_set(voe_3d_models *models, const char *path,
				const voe_assets_material_file *material)
{
	VOE_BASE_ASSERT(models != NULL && path != NULL,
			"setting a material needs a store and a path");
	VOE_BASE_ASSERT(material != NULL, "setting no material");
	const uint32_t index = voe_3d_models_index(models, path);

	if (index == VOE_3D_MODELS || !models->entries[index].loaded ||
	    !models->entries[index].material)
		return;
	take_values(material, &models->entries[index].parts[0].material);
	models->held[index].write = true;
}

void voe_3d_models_material_frame(voe_3d_models *models,
				  voe_render_device *device)
{
	VOE_BASE_ASSERT(models != NULL, "a material frame needs a store");
	VOE_BASE_ASSERT(device != NULL, "a material frame needs a device");

	for (uint32_t i = 0; i < models->count; i++) {
		const voe_3d_model_part *part = &models->entries[i].parts[0];
		voe_3d_material twin = part->material;

		if (!models->held[i].write)
			continue;
		VOE_BASE_ASSERT(models->entries[i].material,
				"a write mark only on a loaded material");
		voe_render_shading_write(device, part->material.shading,
					 voe_3d_material_values(&part->material));
		twin.alpha_mode = VOE_RENDER_ALPHA_BLENDED;
		voe_render_shading_write(device, part->faded,
					 voe_3d_material_values(&twin));
		models->held[i].write = false;
	}
}
