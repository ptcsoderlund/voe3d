// The camera component: its key, where it looks, and the view matrix that comes
// out of that. No writes in this file — those are camera_system.c's.
//
// THE VIEW MATRIX IS A LOOK-AT AND NOT AN INVERTED TRANSFORM. The three basis
// vectors go into the rows and the translation is minus the dot of each with the
// eye, which is the inverse of a rotation-and-translation written out rather
// than computed — nine multiplies instead of a general inverse, and no
// determinant to be near zero.
//
// -Z IS FORWARD, SO THE MATRIX'S THIRD ROW IS THE NEGATIVE OF IT. That is the
// engine's convention and glTF's (CLAUDE.md); a camera that looked along +Z
// would put every model behind it.
#include <base/assert.h>
#include <scene/camera_component.h>

#include <math.h>

const struct voe_ecs_key voe_scene_camera_key = { "voe_scene_camera" };

voe_math_float3 voe_scene_camera_forward(voe_scene_camera camera)
{
	// Zero yaw looks along -Z and a positive yaw turns towards -X, which is
	// the direction a positive rotation about +Y goes in this engine —
	// math/tests/quat.c is what says so.
	float flat = cosf(camera.pitch);

	return (voe_math_float3){ -sinf(camera.yaw) * flat, sinf(camera.pitch),
				  -cosf(camera.yaw) * flat };
}

voe_math_float4x4 voe_scene_camera_view(voe_scene_camera camera)
{
	voe_math_float3 up = { 0.0f, 1.0f, 0.0f };
	voe_math_float3 forward = voe_scene_camera_forward(camera);
	// z is backwards, away from what the camera looks at, because the camera
	// looks along its own -Z.
	voe_math_float3 z = voe_math_float3_neg(forward);
	voe_math_float3 x = voe_math_float3_normalize(
		voe_math_float3_cross(up, z));
	voe_math_float3 y = voe_math_float3_cross(z, x);
	voe_math_float4x4 view = { 0 };

	view.m[0][0] = x.x;
	view.m[0][1] = x.y;
	view.m[0][2] = x.z;
	view.m[0][3] = -voe_math_float3_dot(x, camera.eye);

	view.m[1][0] = y.x;
	view.m[1][1] = y.y;
	view.m[1][2] = y.z;
	view.m[1][3] = -voe_math_float3_dot(y, camera.eye);

	view.m[2][0] = z.x;
	view.m[2][1] = z.y;
	view.m[2][2] = z.z;
	view.m[2][3] = -voe_math_float3_dot(z, camera.eye);

	view.m[3][3] = 1.0f;
	return view;
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
