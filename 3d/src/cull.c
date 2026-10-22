// The view's planes from the rows of projection × view, each normalised; the
// sphere's test against them; a sphere's radius under a world matrix.
//
// THE ROWS, WRITTEN OUT. With M = projection × view and r0..r3 its rows, a
// point p is inside when, in clip space, -w ≤ x ≤ w, -w ≤ y ≤ w and 0 ≤ z ≤ w
// (reversed depth: the near plane at z = w, the far one at z = 0). Each of
// those six is one row sum dotted with (p, 1) being at least nought:
//
//     left r3 + r0   right r3 - r0   bottom r3 + r1   top r3 - r1
//     near r3 - r2   far   r2
//
// so the planes need no inverse and hold for a lens or a box alike.
#include "cull.h"

#include <base/assert.h>

#include <math.h>
#include <stddef.h>

static voe_math_float4 row_of(voe_math_float4x4 m, int row)
{
	return (voe_math_float4){ m.m[row][0], m.m[row][1], m.m[row][2],
				  m.m[row][3] };
}

// A plane scaled so its normal is unit and a distance from it is in metres.
static voe_math_float4 plane_normalised(voe_math_float4 plane)
{
	voe_math_float3 normal = { plane.x, plane.y, plane.z };
	float length = voe_math_float3_length(normal);

	VOE_BASE_ASSERT(length > 0.0f, "a view whose plane has no normal");
	plane = voe_math_float4_scale(plane, 1.0f / length);
	VOE_BASE_ASSERT(isfinite(plane.w), "a view plane at no distance");
	return plane;
}

struct voe_3d_cull voe_3d_cull_of(voe_render_view view, bool sun_side_open)
{
	voe_math_float4x4 m = voe_math_float4x4_mul(view.projection, view.view);
	voe_math_float4 x = row_of(m, 0);
	voe_math_float4 y = row_of(m, 1);
	voe_math_float4 z = row_of(m, 2);
	voe_math_float4 w = row_of(m, 3);
	voe_math_float4 sides[6] = {
		voe_math_float4_add(w, x), voe_math_float4_sub(w, x),
		voe_math_float4_add(w, y), voe_math_float4_sub(w, y),
		voe_math_float4_sub(w, z), z,
	};
	struct voe_3d_cull cull = { .count = 0 };

	for (uint32_t i = 0; i < 6; i++) {
		if (sun_side_open && i == 4)
			continue;
		cull.planes[cull.count++] = plane_normalised(sides[i]);
	}
	VOE_BASE_ASSERT(cull.count == (sun_side_open ? 5u : 6u),
			"a view with planes missing");
	return cull;
}

bool voe_3d_cull_sees(const struct voe_3d_cull *cull, voe_math_float3 centre,
		      float radius)
{
	VOE_BASE_ASSERT(cull != NULL && cull->count <= 6, "no planes to test");
	VOE_BASE_ASSERT(radius >= 0.0f, "a sphere of negative radius");

	for (uint32_t i = 0; i < cull->count; i++) {
		voe_math_float4 plane = cull->planes[i];
		voe_math_float3 normal = { plane.x, plane.y, plane.z };

		if (voe_math_float3_dot(normal, centre) + plane.w < -radius)
			return false;
	}
	return true;
}

float voe_3d_cull_radius(voe_math_float4x4 world, float radius)
{
	float longest = 0.0f;

	VOE_BASE_ASSERT(radius >= 0.0f, "a sphere of negative radius");
	for (int column = 0; column < 3; column++) {
		voe_math_float3 axis = { world.m[0][column], world.m[1][column],
					 world.m[2][column] };

		longest = fmaxf(longest, voe_math_float3_length(axis));
	}
	VOE_BASE_ASSERT(isfinite(longest), "a world matrix that is not finite");
	return radius * longest;
}
