// The sun's basis, the snap to whole texels in double, and the orthographic
// look from the sun, shared by the cascades and the bounce grid. See
// light_box.h for why each step is there.
#include "light_box.h"

#include <3d/projection.h>
#include <3d/shadow_cascades.h>
#include <base/assert.h>
#include <math/float4x4.h>

#include <math.h>

struct voe_3d_light_basis voe_3d_light_box_basis(voe_math_float3 direction)
{
	voe_math_float3 world_up = { 0.0f, 1.0f, 0.0f };
	struct voe_3d_light_basis basis = { .forward = direction };

	if (fabsf(direction.y) > 0.99f)
		world_up = (voe_math_float3){ 0.0f, 0.0f, 1.0f };
	basis.right = voe_math_float3_normalize(
		voe_math_float3_cross(direction, world_up));
	basis.up = voe_math_float3_cross(basis.right, direction);
	VOE_BASE_ASSERT(isfinite(basis.right.x) && isfinite(basis.up.y),
			"a light basis that is not one");
	return basis;
}

// The centre moved to the nearest whole texel of the light's two axes, the
// axes taken through the world origin in double; returned eye-relative.
voe_math_float3 voe_3d_light_box_snap(voe_math_float3 centre, voe_math_double3 eye,
				      struct voe_3d_light_basis basis, double texel)
{
	voe_math_double3 world = voe_math_double3_add(
		eye, voe_math_double3_from_float3(centre));
	voe_math_double3 right = voe_math_double3_from_float3(basis.right);
	voe_math_double3 up = voe_math_double3_from_float3(basis.up);
	double across = world.x * right.x + world.y * right.y + world.z * right.z;
	double upward = world.x * up.x + world.y * up.y + world.z * up.z;
	float shift_across = (float)(round(across / texel) * texel - across);
	float shift_up = (float)(round(upward / texel) * texel - upward);

	VOE_BASE_ASSERT(texel > 0.0, "a snap to texels of no size");
	return voe_math_float3_add(
		centre, voe_math_float3_add(
				voe_math_float3_scale(basis.right, shift_across),
				voe_math_float3_scale(basis.up, shift_up)));
}

// Looking along forward from `pull` metres short of the centre: rows right, up
// and -forward, so forward is -Z as every camera's is.
voe_render_view voe_3d_light_box_look(voe_math_float3 centre, float half,
				      float radius, struct voe_3d_light_basis basis)
{
	float pull = radius + VOE_3D_SHADOW_CASTER_REACH;
	voe_render_view light = { .eye = voe_math_float3_sub(
		centre, voe_math_float3_scale(basis.forward, pull)) };
	voe_math_float3 rows[3] = { basis.right, basis.up,
				    voe_math_float3_neg(basis.forward) };

	for (int row = 0; row < 3; row++) {
		light.view.m[row][0] = rows[row].x;
		light.view.m[row][1] = rows[row].y;
		light.view.m[row][2] = rows[row].z;
		light.view.m[row][3] = -voe_math_float3_dot(rows[row], centre);
	}
	light.view.m[2][3] -= pull;
	light.view.m[3][3] = 1.0f;
	light.projection = voe_3d_projection_orthographic(half, half, 0.0f,
							  pull + radius);
	VOE_BASE_ASSERT(light.reserved == 0.0f, "a light view with padding written");
	return light;
}
