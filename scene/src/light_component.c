// The light component: its key, the reads, and the two conversions between a
// rotation and the direction it shines. Everything that writes one is in
// light_system.c, which is what makes a file including only this header
// provably a reader.
#include <base/assert.h>
#include <scene/light_component.h>

// Below this, 1 - z of a unit direction is too close to +Z for the arc's axis
// to mean anything, and the half turn is taken instead.
#define ALONG_PLUS_Z 1e-6f

const struct voe_ecs_key voe_scene_light_key = { "voe_scene_light" };

// In the counts' own order: the index is the count.
const char *const voe_scene_light_bounces_names[VOE_SCENE_LIGHT_BOUNCES_MAX +
						1] = { "0", "1" };

const voe_scene_light *voe_scene_light_get(const voe_ecs_world *world,
					   voe_ecs_entity entity)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "reading a light out of no world");

	return voe_ecs_component_get(
		world, voe_ecs_component_type(world, &voe_scene_light_key),
		entity);
}

uint32_t voe_scene_light_count(const voe_ecs_world *world)
{
	return voe_ecs_component_count(
		world, voe_ecs_component_type(world, &voe_scene_light_key));
}

const voe_scene_light *voe_scene_light_rows(const voe_ecs_world *world)
{
	return voe_ecs_component_rows(
		world, voe_ecs_component_type(world, &voe_scene_light_key));
}

const voe_ecs_entity *voe_scene_light_entities(const voe_ecs_world *world)
{
	return voe_ecs_component_entities(
		world, voe_ecs_component_type(world, &voe_scene_light_key));
}

// -Z turned by the unit rotation: the negated third column of its matrix.
voe_math_float3 voe_scene_light_direction(voe_math_quat rotation)
{
	voe_math_quat q = voe_math_quat_normalize(rotation);

	return (voe_math_float3){
		-2.0f * (q.x * q.z + q.w * q.y),
		-2.0f * (q.y * q.z - q.w * q.x),
		-(1.0f - 2.0f * (q.x * q.x + q.y * q.y)),
	};
}

// From -Z to d the shortest arc is about -Z x d = (d.y, -d.x, 0), and the
// half-angle quaternion is that axis with w = 1 + dot(-Z, d) = 1 - d.z, then
// normalized.
voe_math_quat voe_scene_light_facing(voe_math_float3 direction)
{
	voe_math_float3 d;

	VOE_BASE_ASSERT(voe_math_float3_length(direction) > 0.0f,
			"facing no direction — see scene/light_component.h");

	d = voe_math_float3_normalize(direction);
	if (1.0f - d.z < ALONG_PLUS_Z)
		return (voe_math_quat){ 0.0f, 1.0f, 0.0f, 0.0f };
	return voe_math_quat_normalize(
		(voe_math_quat){ d.y, -d.x, 0.0f, 1.0f - d.z });
}
