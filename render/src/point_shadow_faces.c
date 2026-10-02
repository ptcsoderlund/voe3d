// The three calls of point_shadow_faces.h: a box-centred sphere over vertex
// positions, that sphere under a world matrix, and the face mask against one
// light's pyramids. Plain arithmetic, no state.
#include "point_shadow_faces.h"

#include <assert.h>
#include <math.h>

voe_math_float4 voe_render_point_shadow_sphere(const voe_render_vertex *vertices,
					       uint32_t count)
{
	assert(vertices != NULL);
	assert(count >= 1);
	voe_math_float3 lo = vertices[0].position;
	voe_math_float3 hi = lo;

	for (uint32_t i = 1; i < count; i++) {
		lo = voe_math_float3_min(lo, vertices[i].position);
		hi = voe_math_float3_max(hi, vertices[i].position);
	}
	const voe_math_float3 centre =
		voe_math_float3_scale(voe_math_float3_add(lo, hi), 0.5f);
	float radius = 0.0f;

	for (uint32_t i = 0; i < count; i++)
		radius = fmaxf(radius, voe_math_float3_length(voe_math_float3_sub(
					       vertices[i].position, centre)));
	assert(radius >= 0.0f);
	return (voe_math_float4){ centre.x, centre.y, centre.z, radius };
}

voe_math_float4 voe_render_point_shadow_sphere_moved(voe_math_float4 sphere,
						     voe_math_float4x4 world)
{
	assert(sphere.w >= 0.0f);
	const voe_math_float3 centre = voe_math_float4x4_transform_point(
		world, (voe_math_float3){ sphere.x, sphere.y, sphere.z });
	float longest = 0.0f;

	for (int j = 0; j < 3; j++)
		longest = fmaxf(longest,
				voe_math_float3_length((voe_math_float3){
					world.m[0][j], world.m[1][j], world.m[2][j] }));
	const float radius = sphere.w * longest;

	assert(radius >= 0.0f);
	return (voe_math_float4){ centre.x, centre.y, centre.z, radius };
}

// Face f looks along axis f / 2, positive when f is even. A side plane such as
// s·x = y has unit normal (s, −1)/√2, so the sphere is on the inner side of it
// moved out by r when s·c_a − |c_b| ≥ −r√2; both other axes must pass.
uint32_t voe_render_point_shadow_faces(voe_math_float4 sphere,
				       voe_math_float3 light, float range)
{
	assert(sphere.w >= 0.0f);
	assert(range >= 0.0f);
	const float c[3] = { sphere.x - light.x, sphere.y - light.y,
			     sphere.z - light.z };
	const float slack = -sphere.w * 1.41421356f;
	uint32_t mask = 0;

	if (sqrtf(c[0] * c[0] + c[1] * c[1] + c[2] * c[2]) - sphere.w > range)
		return 0;
	for (uint32_t f = 0; f < 6; f++) {
		const uint32_t a = f / 2;
		const float along = (f % 2 == 0) ? c[a] : -c[a];

		if (along - fabsf(c[(a + 1) % 3]) >= slack &&
		    along - fabsf(c[(a + 2) % 3]) >= slack)
			mask |= 1u << f;
	}
	assert(mask <= VOE_RENDER_POINT_SHADOW_ALL_FACES);
	return mask;
}
