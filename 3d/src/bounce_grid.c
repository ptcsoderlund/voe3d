// The fit of the probe volume to the eye: the eye's cell about the world origin
// in double, the grid's lowest cell below it, and its corner about the eye; and
// the relight's sun view of that volume through light_box.h. See
// 3d/bounce_grid.h for why each step is there.
#include "light_box.h"

#include <3d/bounce_grid.h>
#include <base/assert.h>

#include <math.h>

voe_3d_bounce_grid voe_3d_bounce_grid_fit(voe_math_double3 eye)
{
	voe_3d_bounce_grid grid = { 0 };
	double spacing = (double)VOE_RENDER_BOUNCE_SPACING;
	double origin[3] = { eye.x, eye.y, eye.z };
	const int32_t below[3] = { VOE_RENDER_BOUNCE_PROBES_XZ / 2,
				   VOE_3D_BOUNCE_BELOW,
				   VOE_RENDER_BOUNCE_PROBES_XZ / 2 };
	float corner[3];

	VOE_BASE_ASSERT(isfinite(eye.x) && isfinite(eye.y) && isfinite(eye.z),
			"an eye that is not a number");
	for (int axis = 0; axis < 3; axis++) {
		grid.cell[axis] = (int32_t)floor(origin[axis] / spacing) - below[axis];
		corner[axis] = (float)((double)grid.cell[axis] * spacing - origin[axis]);
	}
	grid.corner = (voe_math_float3){ corner[0], corner[1], corner[2] };
	VOE_BASE_ASSERT(isfinite(grid.corner.x) && isfinite(grid.corner.y) &&
				isfinite(grid.corner.z),
			"a corner that is not a number");
	return grid;
}

// A sphere about the volume's centre reaching every surface a probe sees,
// snapped to whole texels and looked at from the sun as a cascade is.
voe_render_view voe_3d_bounce_grid_sun(voe_3d_bounce_grid grid,
				       voe_math_double3 eye,
				       voe_math_float3 direction)
{
	voe_math_float3 half_sides = {
		0.5f * (float)VOE_RENDER_BOUNCE_PROBES_XZ * VOE_RENDER_BOUNCE_SPACING,
		0.5f * (float)VOE_RENDER_BOUNCE_PROBES_Y * VOE_RENDER_BOUNCE_SPACING,
		0.5f * (float)VOE_RENDER_BOUNCE_PROBES_XZ * VOE_RENDER_BOUNCE_SPACING
	};
	float radius = voe_math_float3_length(half_sides) + VOE_RENDER_BOUNCE_REACH;
	double texel = 2.0 * (double)radius / (double)VOE_RENDER_BOUNCE_SHADOW_TEXELS;
	struct voe_3d_light_basis basis;
	voe_math_float3 centre;

	VOE_BASE_ASSERT(fabsf(voe_math_float3_length(direction) - 1.0f) < 1e-3f,
			"a sun whose direction is not unit length");
	basis = voe_3d_light_box_basis(direction);
	centre = voe_3d_light_box_snap(voe_math_float3_add(grid.corner, half_sides),
				       eye, basis, texel);
	return voe_3d_light_box_look(centre, radius, radius, basis);
}
