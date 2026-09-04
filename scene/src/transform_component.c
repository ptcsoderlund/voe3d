// The transform component: its key, the matrix its three numbers become, and the
// reads anyone may do.
//
// NOTHING IN THIS FILE WRITES. Every write to this table is in
// transform_system.c, which is the whole of what "read by anyone, written by
// one" costs to arrange.
#include <base/assert.h>
#include <scene/transform_component.h>

const struct voe_ecs_key voe_scene_transform_key = { "voe_scene_transform" };

voe_math_float4x4 voe_scene_transform_matrix(voe_scene_transform transform)
{
	// T · R · S. Right to left: scaled, then turned, then moved.
	return voe_math_float4x4_mul(
		voe_math_float4x4_from_translation(transform.position),
		voe_math_float4x4_mul(
			voe_math_float4x4_from_quat(transform.rotation),
			voe_math_float4x4_from_scale(transform.scale)));
}

const voe_scene_transform *voe_scene_transform_get(const voe_ecs_world *world,
						   voe_ecs_entity entity)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "reading a transform out of no world");

	return voe_ecs_component_get(
		world, voe_ecs_component_type(world, &voe_scene_transform_key),
		entity);
}

uint32_t voe_scene_transform_count(const voe_ecs_world *world)
{
	return voe_ecs_component_count(
		world, voe_ecs_component_type(world, &voe_scene_transform_key));
}

const voe_scene_transform *voe_scene_transform_rows(const voe_ecs_world *world)
{
	return voe_ecs_component_rows(
		world, voe_ecs_component_type(world, &voe_scene_transform_key));
}

const voe_ecs_entity *voe_scene_transform_entities(const voe_ecs_world *world)
{
	return voe_ecs_component_entities(
		world, voe_ecs_component_type(world, &voe_scene_transform_key));
}
