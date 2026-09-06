// The material component: its key, the upload that turns its numbers into a
// record `render` holds, its creation call, and the reads the draw system does.
//
// THE UPLOAD IS WHERE THE TWO SIDES MEET, AND IT IS A STRAIGHT COPY OF FIELDS.
// Every texture id loses its generation half on the way — the shader indexes
// with the index and the generation was only ever for catching a stale id on
// this side — and nothing else changes shape. A record and a component are two
// spellings of the same numbers, one for a person to author and one for the GPU
// to read.
#include <3d/material_component.h>
#include <base/assert.h>

const struct voe_ecs_key voe_3d_material_key = { "voe_3d_material" };

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

	VOE_BASE_ASSERT(device != NULL, "uploading a material to no device");
	VOE_BASE_ASSERT(material != NULL, "uploading nothing as a material");

	values = (voe_render_shading_values){
		.base_colour = material->base_colour,
		.metallic = material->metallic,
		.roughness = material->roughness,
		// The enum crosses as the uint the record holds it in, which is
		// the one field here that changes type rather than only shape.
		.alpha_mode = (uint32_t)material->alpha_mode,
		.alpha_cutoff = material->alpha_cutoff,
		.emissive = material->emissive,
		.base_colour_texture = material->base_colour_texture.index,
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
