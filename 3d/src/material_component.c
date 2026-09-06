// The material component: its key, the upload that turns its numbers into a
// record `render` holds, its creation call, and the reads the draw system does.
//
// THE UPLOAD IS WHERE THE TWO SIDES MEET, AND IT IS A STRAIGHT COPY OF FIELDS.
// Every texture id loses its generation half on the way — the shader indexes
// with the index and the generation was only ever for catching a stale id on
// this side — and nothing else changes shape. A record and a component are two
// spellings of the same numbers, one for a person to author and one for the GPU
// to read.
//
// THE ONE FIELD THAT IS NOT A STRAIGHT COPY IS THE UV RECT, and it is not
// because C zeroes what a designated initialiser does not name. Every other
// field's zero is the answer somebody would have wanted — opaque, lit, no
// texture — and a scale of nothing is not: it reads one texel across the whole
// surface. So a zero here is read as "the whole texture", which is what every
// material written before this field existed meant.
#include <3d/material_component.h>
#include <base/assert.h>

const struct voe_ecs_key voe_3d_material_key = { "voe_3d_material" };

// The rect as the record wants it — xy the offset, zw the scale — after a scale
// of nothing has been read as the whole texture.
//
// COMPONENT BY COMPONENT AND NOT ALL OR NOTHING, because half a rect is not a
// case worth a rule of its own: a material that scaled x and forgot y meant to
// read the whole of y. It writes the answer back so that the component and the
// record it just made cannot disagree — see the header.
static voe_math_float4 uv_rect_of(voe_3d_material *material)
{
	if (material->base_colour_uv_scale.x == 0.0f)
		material->base_colour_uv_scale.x = 1.0f;
	if (material->base_colour_uv_scale.y == 0.0f)
		material->base_colour_uv_scale.y = 1.0f;

	return (voe_math_float4){
		material->base_colour_uv_offset.x,
		material->base_colour_uv_offset.y,
		material->base_colour_uv_scale.x,
		material->base_colour_uv_scale.y,
	};
}

void voe_3d_material_register(voe_ecs_world *world, uint32_t capacity)
{
	VOE_BASE_ASSERT(world != NULL, "registering materials in no world");

	(void)voe_ecs_component_register(world, &voe_3d_material_key,
					 sizeof(voe_3d_material), capacity);
}

bool voe_3d_material_upload(voe_render_device *device,
			    voe_3d_material *material, voe_base_error *error)
{
	voe_render_shading_values values;
	voe_math_float4 rect;

	VOE_BASE_ASSERT(device != NULL, "uploading a material to no device");
	VOE_BASE_ASSERT(material != NULL, "uploading nothing as a material");

	// Before the record below and not inside it: this is the one call here
	// that writes to the material, and reading the rest of it in the same
	// initialiser would leave a reader working out whether that matters.
	rect = uv_rect_of(material);

	values = (voe_render_shading_values){
		.base_colour = material->base_colour,
		.metallic = material->metallic,
		.roughness = material->roughness,
		// The enum crosses as the uint the record holds it in, which is
		// the one field here that changes type rather than only shape.
		.alpha_mode = (uint32_t)material->alpha_mode,
		.alpha_cutoff = material->alpha_cutoff,
		.emissive = material->emissive,
		.unlit = material->unlit ? 1u : 0u,
		.base_colour_texture = material->base_colour_texture.index,
		.base_colour_distance_field =
			material->base_colour_distance_field ? 1u : 0u,
		.base_colour_uv_rect = rect,
		.metallic_roughness_texture =
			material->metallic_roughness_texture.index,
		.normal_texture = material->normal_texture.index,
		.occlusion_texture = material->occlusion_texture.index,
		.emissive_texture = material->emissive_texture.index,
	};

	return voe_render_shading_create(device, values, &material->shading,
					 error);
}

bool voe_3d_material_add(voe_ecs_world *world, voe_ecs_entity entity,
			 voe_3d_material material)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "adding a material to no world");

	return voe_ecs_component_add(
		world, voe_ecs_component_type(world, &voe_3d_material_key),
		entity, &material);
}

const voe_3d_material *voe_3d_material_get(const voe_ecs_world *world,
					   voe_ecs_entity entity)
{
	return voe_ecs_component_get(
		world, voe_ecs_component_type(world, &voe_3d_material_key),
		entity);
}

uint32_t voe_3d_material_count(const voe_ecs_world *world)
{
	return voe_ecs_component_count(
		world, voe_ecs_component_type(world, &voe_3d_material_key));
}

const voe_3d_material *voe_3d_material_rows(const voe_ecs_world *world)
{
	return voe_ecs_component_rows(
		world, voe_ecs_component_type(world, &voe_3d_material_key));
}

const voe_ecs_entity *voe_3d_material_entities(const voe_ecs_world *world)
{
	return voe_ecs_component_entities(
		world, voe_ecs_component_type(world, &voe_3d_material_key));
}
