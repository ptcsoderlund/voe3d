// The model store's materials (3d/models.h; 0399 points 2, 4 and 5): a
// `.material`'s values and its four maps uploaded as one part with no
// geometry, its record and its blended twin; a set taking new values and
// marking the entry; the frame rewriting every marked entry's two records.
// The values' cutout, emission and double_sided are 0420 point 2's.
//
// Used by the editor and the game: the load when a `.material` is read or a
// map of it changes, the set while a field is dragged, the frame once a frame
// before its first pass. The load decodes each map and then puts; the put
// alone takes maps the loader's decoding step decoded (0459 point 1).
//
// Constraints: a map's picture is decoded by its path's extension, as a
// picture entry's is, so a map path that is no `.png`, `.jpg` or `.jpeg` fails
// MALFORMED. The ORM map (0400) is uploaded once, as DATA, and its one texture
// set in both the metal-roughness slot (green, blue) and the occlusion slot
// (red): one picture, one fetch, so it is held and freed once. An empty ORM
// map leaves both slots none. The emission map goes up COLOUR into the emissive
// slot. Each material uploads its own maps; a map shared by two materials
// costs two textures, which a store of pictures by path would lift. The set
// and the frame walk every entry. A failed load says its cause once, at the
// step that met it (0445), the material's path being the caller's subject.
#include "model_picture.h"
#include "models_store.h"
#include "picture_upload.h"

#include <3d/material_component.h>
#include <3d/models.h>
#include <assets/image.h>
#include <assets/material.h>
#include <base/arena.h>
#include <base/assert.h>
#include <base/report.h>

#include <stdio.h>
#include <string.h>

// A material's maps a load uploads at most.
#define MAPS 4
#define SCRATCH_BLOCK (1024 * 1024)

// `from`'s values on a material, the maps aside: a cutout above 0 is CUTOUT at
// that cutoff, else OPAQUE; emission is the colour times its strength.
static void take_values(const voe_assets_material_file *from, voe_3d_material *to)
{
	to->base_colour = (voe_math_float4){ from->colour[0], from->colour[1],
					     from->colour[2], 1.0f };
	to->metallic = from->metal;
	to->roughness = from->roughness;
	to->uv_repeat = from->repeat;
	to->unlit = from->shader == VOE_ASSETS_MATERIAL_UNLIT;
	to->alpha_mode = from->cutout > 0.0f ? VOE_RENDER_ALPHA_CUTOUT :
					       VOE_RENDER_ALPHA_OPAQUE;
	to->alpha_cutoff = from->cutout;
	to->emissive = (voe_math_float3){
		from->emission[0] * from->emission_strength,
		from->emission[1] * from->emission_strength,
		from->emission[2] * from->emission_strength
	};
	to->double_sided = from->double_sided;
}

static voe_3d_material material_of(const voe_assets_material_file *from)
{
	const voe_render_texture none = { .index = VOE_RENDER_NO_TEXTURE };
	voe_3d_material material = {
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

// `map`'s picture, the `name` map at `path`, decoded into `image`; width 0
// for a map of no bytes. MALFORMED when it will not decode: a path that is no
// picture said here, a decoder's cause under "the <name> map `path`" as its
// subject (0445 point 1).
static bool decode_map(voe_base_arena *scratch, const char *name,
		       const char *path, voe_3d_material_map map,
		       voe_assets_image *image, voe_base_error *error)
{
	char subject[VOE_ASSETS_MATERIAL_PATH + 32];
	bool decoded;

	*image = (voe_assets_image){ 0 };
	if (map.size == 0)
		return true;
	VOE_BASE_ASSERT(map.bytes != NULL, "a map of bytes at no address");
	if (!voe_3d_model_picture_is(path)) {
		VOE_BASE_ERROR("3d",
			       "the %s map `%s` is not a PNG or JPEG picture; save it as PNG",
			       name, path);
		if (error != NULL)
			*error = VOE_BASE_ERROR_MALFORMED;
		return false;
	}
	(void)snprintf(subject, sizeof(subject), "the %s map `%s`", name, path);
	voe_base_report_subject_push(subject);
	decoded = voe_3d_model_picture_decode(path, map.bytes, map.size,
					      scratch, image, error);
	voe_base_report_subject_pop();
	if (!decoded && error != NULL)
		*error = VOE_BASE_ERROR_MALFORMED;
	return decoded;
}

// `image` uploaded as `kind` into `slot`, in whichever shape it came
// (picture_upload.h); nothing for one of width 0.
static bool upload_map(voe_render_device *device, const voe_assets_image *image,
		       voe_render_texture_kind kind, maps_made *made,
		       voe_render_texture *slot, voe_base_error *error)
{
	if (image->width == 0)
		return true;
	VOE_BASE_ASSERT(image->pixels != NULL, "a map of pixels at no address");
	if (!voe_3d_picture_upload(device, image, kind, slot, error))
		return false;
	made->made[made->count++] = *slot;
	return true;
}

// The maps, the record and its twin into `entry`; on failure what was made is
// given back.
static bool upload_material(voe_render_device *device,
			    const voe_assets_material_file *values,
			    const voe_3d_material_pictures *pictures,
			    voe_3d_model_entry *entry, entry_held *held,
			    voe_base_error *error)
{
	voe_3d_model_part part = { .material = material_of(values),
				   .source = VOE_ASSETS_MODEL_NONE };
	voe_3d_material *material = &part.material;
	maps_made made = { 0 };
	bool record = false;

	bool uploaded =
		upload_map(device, &pictures->colour, VOE_RENDER_TEXTURE_COLOUR,
			   &made, &material->base_colour_texture, error) &&
		upload_map(device, &pictures->normal, VOE_RENDER_TEXTURE_DATA,
			   &made, &material->normal_texture, error) &&
		upload_map(device, &pictures->orm, VOE_RENDER_TEXTURE_DATA,
			   &made, &material->metallic_roughness_texture,
			   error) &&
		upload_map(device, &pictures->emission,
			   VOE_RENDER_TEXTURE_COLOUR, &made,
			   &material->emissive_texture, error);

	material->occlusion_texture = material->metallic_roughness_texture;
	uploaded = uploaded &&
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

bool voe_3d_models_put_material(voe_3d_models *models,
				voe_render_device *device, const char *path,
				uint64_t stamp,
				const voe_assets_material_file *material,
				voe_3d_material_pictures pictures,
				voe_base_error *error)
{
	voe_3d_model_entry entry;
	entry_held held;
	bool loaded;

	VOE_BASE_ASSERT(models != NULL && device != NULL && path != NULL,
			"putting a material needs a store, a device and a path");
	VOE_BASE_ASSERT(material != NULL, "putting no material");

	if (!voe_3d_models_room(models, path, error))
		return false;
	voe_3d_models_entry_new(path, stamp, &entry, &held);
	loaded = upload_material(device, material, &pictures, &entry, &held,
				 error);
	(void)voe_3d_models_keep(models, device, &entry, &held);
	return loaded;
}

// Each map decoded on `scratch` in the order the put uploads them, stopping at
// the first that will not decode, so a failure is said once.
static bool decode_maps(voe_base_arena *scratch,
			const voe_assets_material_file *material,
			voe_3d_material_maps maps,
			voe_3d_material_pictures *pictures, voe_base_error *error)
{
	return decode_map(scratch, "colour", material->colormap, maps.colour,
			  &pictures->colour, error) &&
	       decode_map(scratch, "normal", material->normalmap, maps.normal,
			  &pictures->normal, error) &&
	       decode_map(scratch, "ORM", material->ormmap, maps.orm,
			  &pictures->orm, error) &&
	       decode_map(scratch, "emission", material->emissionmap,
			  maps.emission, &pictures->emission, error);
}

bool voe_3d_models_load_material(voe_3d_models *models,
				 voe_render_device *device, const char *path,
				 uint64_t stamp,
				 const voe_assets_material_file *material,
				 voe_3d_material_maps maps,
				 voe_base_error *error)
{
	voe_3d_material_pictures pictures;
	voe_base_arena *scratch;
	bool loaded;

	VOE_BASE_ASSERT(models != NULL && device != NULL && path != NULL,
			"loading a material needs a store, a device and a path");
	VOE_BASE_ASSERT(material != NULL, "loading no material");

	if (!voe_3d_models_room(models, path, error))
		return false;
	scratch = voe_base_arena_new(SCRATCH_BLOCK);
	if (decode_maps(scratch, material, maps, &pictures, error)) {
		loaded = voe_3d_models_put_material(models, device, path, stamp,
						    material, pictures, error);
	} else {
		voe_3d_models_fail(models, path, stamp);
		loaded = false;
	}
	voe_base_arena_destroy(scratch);
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
