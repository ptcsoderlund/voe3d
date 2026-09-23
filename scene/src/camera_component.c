// The camera component: its key, the reads, and the view a pose gives. No writes
// in this file — those are camera_system.c's.
//
// THE VIEW IS A GENERAL INVERSE AND NOT A LOOK-AT, because the pose may carry
// scale and roll (0222) and a look-at written out assumes neither. Its one cost
// is a determinant that can be nought, which is the false the view returns.
#include <base/assert.h>
#include <scene/camera_component.h>

const struct voe_ecs_key voe_scene_camera_key = { "voe_scene_camera" };

// The determinant is the test voe_math_float4x4_inverse asserts on, asked first
// so a singular pose is a false here and never that assert.
bool voe_scene_camera_view(voe_scene_transform pose, voe_math_float4x4 *out)
{
	voe_math_float4x4 matrix = voe_scene_transform_matrix(pose);

	VOE_BASE_DEBUG_ASSERT(out != NULL, "a view written to nowhere");

	if (voe_math_float4x4_determinant(matrix) == 0.0f)
		return false;
	*out = voe_math_float4x4_inverse(matrix);
	return true;
}

const voe_scene_camera *voe_scene_camera_get(const voe_ecs_world *world,
					     voe_ecs_entity entity)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "reading a camera out of no world");

	return voe_ecs_component_get(
		world, voe_ecs_component_type(world, &voe_scene_camera_key),
		entity);
}

uint32_t voe_scene_camera_count(const voe_ecs_world *world)
{
	return voe_ecs_component_count(
		world, voe_ecs_component_type(world, &voe_scene_camera_key));
}

const voe_scene_camera *voe_scene_camera_rows(const voe_ecs_world *world)
{
	return voe_ecs_component_rows(
		world, voe_ecs_component_type(world, &voe_scene_camera_key));
}

const voe_ecs_entity *voe_scene_camera_entities(const voe_ecs_world *world)
{
	return voe_ecs_component_entities(
		world, voe_ecs_component_type(world, &voe_scene_camera_key));
}
