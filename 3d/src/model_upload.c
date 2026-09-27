// A read model's pictures and materials uploaded through `render`, moved here
// whole from import.c (3d/src/model_upload.h says why and what the colour spaces
// are). Pictures first, because a material names texture ids; then one shading
// record per glTF material, and one for glTF's default when a primitive needs it.
//
// THE DEFAULT IS UPLOADED ONCE AND SHARED by every primitive that names no
// material, as a glTF material is shared by the primitives naming it: it is one
// material, so it is one record.
//
// Every index here came out of a file, so each is checked where it reaches an
// array rather than trusted from `assets` a folder away.
#include "model_upload.h"

#include <base/assert.h>

// Says a picture is wanted, ignoring the indices that name none.
static void wanted(bool *marks, uint32_t count, uint32_t index)
{
	if (index < count)
		marks[index] = true;
}

// Uploads picture `i` in `kind` into `id` and lists it.
static bool upload_picture(voe_render_device *device,
			   const voe_assets_model *model, uint32_t i,
			   voe_render_texture_kind kind, voe_render_texture *id,
			   voe_3d_model_upload *out, voe_base_error *error)
{
	if (!voe_render_texture_create(device, kind, VOE_RENDER_SAMPLING_SMOOTH,
				       model->images[i].width,
				       model->images[i].height,
				       model->images[i].pixels, id, error))
		return false;
	out->textures[out->texture_count++] = *id;
	return true;
}

// A texture id per picture, in each colour space it is wanted in; a slot left
// zeroed is a picture never wanted that way round, which samples white.
static bool upload_images(voe_render_device *device, voe_base_arena *arena,
			  const voe_assets_model *model,
			  voe_render_texture *colours, voe_render_texture *data,
			  voe_3d_model_upload *out, voe_base_error *error)
{
	const uint32_t count = model->image_count;
	bool *as_colour = voe_base_arena_push(arena, count * sizeof(*as_colour));
	bool *as_data = voe_base_arena_push(arena, count * sizeof(*as_data));

	for (uint32_t i = 0; i < model->material_count; i++) {
		const voe_assets_material *material = &model->materials[i];

		wanted(as_colour, count, material->base_colour_image);
		wanted(as_colour, count, material->emissive_image);
		wanted(as_data, count, material->metallic_roughness_image);
		wanted(as_data, count, material->normal_image);
		wanted(as_data, count, material->occlusion_image);
	}
	for (uint32_t i = 0; i < count; i++) {
		if (as_colour[i] &&
		    !upload_picture(device, model, i, VOE_RENDER_TEXTURE_COLOUR,
				    &colours[i], out, error))
			return false;
		if (as_data[i] &&
		    !upload_picture(device, model, i, VOE_RENDER_TEXTURE_DATA,
				    &data[i], out, error))
			return false;
	}
	return true;
}

// The id of the picture at `index`, or the "there isn't one" id: a material
// naming no picture and one naming a picture never uploaded both sample white.
static voe_render_texture texture_at(const voe_assets_model *model,
				     const voe_render_texture *ids,
				     uint32_t index)
{
	voe_render_texture none = { .index = VOE_RENDER_NO_TEXTURE,
				    .generation = 0 };

	if (index == VOE_ASSETS_MODEL_NONE || index >= model->image_count)
		return none;
	return ids[index];
}

// `assets`' three words into `render`'s three words. Two enums and not one
// because `assets` may not name `render` — see 3d/material_component.h. It is
// one to one and total, so the default below is unreachable rather than a
// fallback: the reader refuses a mode it does not know (assets/model.h).
static voe_render_alpha_mode alpha_mode_of(voe_assets_alpha_mode mode)
{
	switch (mode) {
	case VOE_ASSETS_ALPHA_CUTOUT:
		return VOE_RENDER_ALPHA_CUTOUT;
	case VOE_ASSETS_ALPHA_BLENDED:
		return VOE_RENDER_ALPHA_BLENDED;
	case VOE_ASSETS_ALPHA_OPAQUE:
		break;
	}
	return VOE_RENDER_ALPHA_OPAQUE;
}

// Uploads `material` and lists its shading record.
static bool upload_material(voe_render_device *device,
			    voe_3d_material *material,
			    voe_3d_model_upload *out, voe_base_error *error)
{
	if (!voe_3d_material_upload(device, material, error))
		return false;
	out->shadings[out->shading_count++] = material->shading;
	return true;
}

static bool upload_materials(voe_render_device *device,
			     const voe_assets_model *model,
			     const voe_render_texture *colours,
			     const voe_render_texture *data,
			     voe_3d_model_upload *out, voe_base_error *error)
{
	for (uint32_t i = 0; i < model->material_count; i++) {
		const voe_assets_material *from = &model->materials[i];

		out->materials[i] = (voe_3d_material){
			.base_colour = from->base_colour,
			.metallic = from->metallic,
			.roughness = from->roughness,
			.emissive = from->emissive,
			.alpha_mode = alpha_mode_of(from->alpha_mode),
			.alpha_cutoff = from->alpha_cutoff,
			.base_colour_texture = texture_at(
				model, colours, from->base_colour_image),
			.metallic_roughness_texture = texture_at(
				model, data, from->metallic_roughness_image),
			.normal_texture =
				texture_at(model, data, from->normal_image),
			.occlusion_texture =
				texture_at(model, data, from->occlusion_image),
			.emissive_texture = texture_at(model, colours,
						       from->emissive_image),
		};
		if (!upload_material(device, &out->materials[i], out, error))
			return false;
		out->material_count = i + 1;
	}
	return true;
}

// Whether a primitive names no material, or one past the file's: glTF says
// such a primitive wears the default material.
static bool needs_default(const voe_assets_model *model)
{
	for (uint32_t i = 0; i < model->primitive_count; i++) {
		if (model->primitives[i].material >= model->material_count)
			return true;
	}
	return false;
}

// glTF's default material: white, fully metallic, fully rough, opaque, no
// pictures. It is what an unmaterialled primitive is *defined* to be, so it is
// the file's answer and not a substitute for one.
static bool upload_default(voe_render_device *device, voe_3d_model_upload *out,
			   voe_base_error *error)
{
	const voe_render_texture none = { .index = VOE_RENDER_NO_TEXTURE };

	out->default_material = (voe_3d_material){
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.metallic = 1.0f,
		.roughness = 1.0f,
		.alpha_mode = VOE_RENDER_ALPHA_OPAQUE,
		.alpha_cutoff = 0.5f,
		.base_colour_texture = none,
		.metallic_roughness_texture = none,
		.normal_texture = none,
		.occlusion_texture = none,
		.emissive_texture = none,
	};
	if (!upload_material(device, &out->default_material, out, error))
		return false;
	out->has_default = true;
	return true;
}

bool voe_3d_model_upload_create(voe_render_device *device,
				voe_base_arena *arena,
				const voe_assets_model *model,
				voe_3d_model_upload *out, voe_base_error *error)
{
	VOE_BASE_ASSERT(device != NULL, "uploading a model with no device");
	VOE_BASE_ASSERT(arena != NULL, "uploading a model without an arena");
	VOE_BASE_ASSERT(model != NULL && out != NULL,
			"uploading no model, or into nothing");

	const uint32_t images = model->image_count;
	voe_render_texture *colours;
	voe_render_texture *data;

	// One more of each than the file can need, so no array is empty.
	*out = (voe_3d_model_upload){
		.materials = voe_base_arena_push(
			arena, (model->material_count + 1) *
				       sizeof(*out->materials)),
		.textures = voe_base_arena_push(
			arena, (2 * images + 1) * sizeof(*out->textures)),
		.shadings = voe_base_arena_push(
			arena, (model->material_count + 1) *
				       sizeof(*out->shadings)),
	};
	colours = voe_base_arena_push(arena, (images + 1) * sizeof(*colours));
	data = voe_base_arena_push(arena, (images + 1) * sizeof(*data));

	if (!upload_images(device, arena, model, colours, data, out, error) ||
	    !upload_materials(device, model, colours, data, out, error))
		return false;
	if (needs_default(model) && !upload_default(device, out, error))
		return false;
	VOE_BASE_ASSERT(out->texture_count <= 2 * images,
			"more pictures uploaded than wanted");
	return true;
}

voe_3d_material voe_3d_model_upload_material(const voe_3d_model_upload *upload,
					     uint32_t material)
{
	VOE_BASE_ASSERT(upload != NULL, "a material from no upload");

	if (material < upload->material_count)
		return upload->materials[material];
	VOE_BASE_ASSERT(upload->has_default,
			"a primitive with no material and no default uploaded");
	return upload->default_material;
}
