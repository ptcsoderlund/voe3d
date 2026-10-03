// The fit of the probe volume to the eye: the eye's cell about the world origin
// in double, the grid's lowest cell below it, and its corner about the eye.
// See 3d/bounce_grid.h for why each step is there.
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
