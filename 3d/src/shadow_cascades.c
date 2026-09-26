// The fit of the sun's four cascades to one view: the splits, each slice's
// sphere, the snap to whole texels in double, and the light's view and box.
// See 3d/shadow_cascades.h for why each step is there.
//
// THE SPHERE IS TAKEN IN VIEW SPACE AND ONLY ITS CENTRE IS TURNED. The slice's
// corners come from the inverted projection, where they are the same numbers
// whichever way the camera faces; the inverted view then takes the centre
// alone to eye-relative world space. Inverting view × projection whole would
// give the same corners with the view's rotation in their float noise.
//
// THE BOX IS A LITTLE WIDER THAN THE SPHERE. Snapping moves the centre by up to
// half a texel on each axis, so the box's half side is radius × texels /
// (texels - 1): exactly enough that the snapped box still holds the sphere.
//
// THE LIGHT VIEW'S TRANSLATION IS TAKEN FROM THE CENTRE, NOT FROM THE PULLED
// BACK EYE. Across and up, the pull along forward is nothing, so the centre's
// few metres give those rows their precision rather than the eye's two hundred.
#include <3d/projection.h>
#include <3d/shadow_cascades.h>
#include <base/assert.h>
#include <math/float4.h>
#include <math/float4x4.h>

#include <math.h>

typedef struct {
	voe_math_float3 right;
	voe_math_float3 up;
	voe_math_float3 forward;
} light_basis;

typedef struct {
	voe_math_float3 centre;
	float radius;
} slice_sphere;

// The practical scheme: each split the blend of the way from the even one to
// the logarithmic one; the last is `last` itself, not powf's rounding of it.
static void split_distances(float near_plane, float last,
			    float splits[VOE_RENDER_SHADOW_CASCADES])
{
	for (int i = 0; i < VOE_RENDER_SHADOW_CASCADES; i++) {
		float t = (float)(i + 1) / (float)VOE_RENDER_SHADOW_CASCADES;
		float logarithmic = near_plane * powf(last / near_plane, t);
		float even = near_plane + (last - near_plane) * t;

		splits[i] = VOE_3D_SHADOW_SPLIT_BLEND * logarithmic +
			    (1.0f - VOE_3D_SHADOW_SPLIT_BLEND) * even;
	}
	splits[VOE_RENDER_SHADOW_CASCADES - 1] = last;
	VOE_BASE_ASSERT(splits[0] > near_plane, "a first split at the eye");
}

// The slice from `from` to `to` metres along the view, its eight corners out
// of the inverted projection (reversed depth at distance d is m23 / d - m22),
// bounded by their centroid and the furthest of them.
static slice_sphere bound_slice(voe_render_view view, voe_math_float4x4 unproject,
				float from, float to)
{
	voe_math_float3 corners[8];
	voe_math_float3 sum = { 0.0f, 0.0f, 0.0f };
	voe_math_float3 centre;
	float furthest = 0.0f;

	for (int i = 0; i < 8; i++) {
		float distance = (i & 4) ? to : from;
		voe_math_float4 clip = {
			(i & 1) ? 1.0f : -1.0f, (i & 2) ? 1.0f : -1.0f,
			view.projection.m[2][3] / distance - view.projection.m[2][2],
			1.0f
		};
		voe_math_float4 point = voe_math_float4x4_mul_float4(unproject, clip);

		corners[i] = (voe_math_float3){ point.x / point.w, point.y / point.w,
						point.z / point.w };
		sum = voe_math_float3_add(sum, corners[i]);
	}
	centre = voe_math_float3_scale(sum, 1.0f / 8.0f);
	for (int i = 0; i < 8; i++) {
		float reach = voe_math_float3_length(
			voe_math_float3_sub(corners[i], centre));

		furthest = fmaxf(furthest, reach);
	}
	VOE_BASE_ASSERT(furthest > 0.0f, "a slice with no size");
	return (slice_sphere){
		.centre = voe_math_float4x4_transform_point(
			voe_math_float4x4_inverse(view.view), centre),
		.radius = (float)(ceil((double)furthest * 100.0) / 100.0),
	};
}

static light_basis basis_of(voe_math_float3 direction)
{
	voe_math_float3 world_up = { 0.0f, 1.0f, 0.0f };
	light_basis basis = { .forward = direction };

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
static voe_math_float3 snap(voe_math_float3 centre, voe_math_double3 eye,
			    light_basis basis, double texel)
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
static voe_render_view look_from_the_sun(voe_math_float3 centre, float half,
					 float radius, light_basis basis)
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

voe_3d_shadow_cascades voe_3d_shadow_cascades_fit(voe_render_view view,
						  voe_math_double3 eye,
						  voe_math_float3 direction,
						  uint32_t texels)
{
	voe_3d_shadow_cascades fit = { 0 };
	float depth_scale = view.projection.m[2][2];
	float depth_offset = view.projection.m[2][3];
	float near_plane = depth_offset / (1.0f + depth_scale);
	float far_plane = depth_offset / depth_scale;
	voe_math_float4x4 unproject = voe_math_float4x4_inverse(view.projection);
	light_basis basis;
	float from = near_plane;

	VOE_BASE_ASSERT(texels > 1u, "cascades of fewer than two texels");
	VOE_BASE_ASSERT(view.projection.m[3][2] == -1.0f && depth_scale > 0.0f,
			"cascades for a view that is not voe_3d_projection's");
	VOE_BASE_ASSERT(fabsf(voe_math_float3_length(direction) - 1.0f) < 1e-3f,
			"a sun whose direction is not unit length");

	basis = basis_of(direction);
	split_distances(near_plane, fminf(far_plane, VOE_3D_SHADOW_REACH),
			fit.shadow.splits);
	for (int i = 0; i < VOE_RENDER_SHADOW_CASCADES; i++) {
		slice_sphere sphere = bound_slice(view, unproject, from,
						  fit.shadow.splits[i]);
		float half = sphere.radius * (float)texels / (float)(texels - 1u);
		float texel = 2.0f * half / (float)texels;
		voe_math_float3 centre = snap(sphere.centre, eye, basis, texel);

		fit.light[i] = look_from_the_sun(centre, half, sphere.radius, basis);
		fit.shadow.cascades[i] = voe_math_float4x4_mul(fit.light[i].projection,
							       fit.light[i].view);
		fit.shadow.texels[i] = texel;
		from = fit.shadow.splits[i];
	}
	fit.shadow.count = VOE_RENDER_SHADOW_CASCADES;
	VOE_BASE_ASSERT(fit.shadow.splits[0] < fit.shadow.splits[1],
			"splits that do not rise");
	return fit;
}
