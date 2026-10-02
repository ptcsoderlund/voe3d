// The fit of the probe volume to one view: forward out of the view, the grid's
// whole cells about the world origin in double, and its corner about the eye.
// See 3d/bounce_grid.h for why each step is there.
#include <3d/bounce_grid.h>
#include <base/assert.h>

#include <math.h>

voe_3d_bounce_grid voe_3d_bounce_grid_fit(voe_render_view view,
					  voe_math_double3 eye)
{
	voe_3d_bounce_grid grid = { 0 };
	voe_math_float3 forward = voe_math_float3_normalize((voe_math_float3){
		-view.view.m[2][0], -view.view.m[2][1], -view.view.m[2][2] });
	double spacing = (double)VOE_RENDER_BOUNCE_SPACING;
	double ahead = (double)VOE_3D_BOUNCE_AHEAD;
	double centre[3] = { eye.x + forward.x * ahead, eye.y + forward.y * ahead,
			     eye.z + forward.z * ahead };
	double origin[3] = { eye.x, eye.y, eye.z };
	const int32_t half[3] = { VOE_RENDER_BOUNCE_PROBES_XZ / 2,
				  VOE_RENDER_BOUNCE_PROBES_Y / 2,
				  VOE_RENDER_BOUNCE_PROBES_XZ / 2 };
	float corner[3];

	VOE_BASE_ASSERT(isfinite(forward.x) && isfinite(forward.y) &&
				isfinite(forward.z),
			"a view with no forward");

	for (int axis = 0; axis < 3; axis++) {
		grid.cell[axis] = (int32_t)floor(centre[axis] / spacing) - half[axis];
		corner[axis] = (float)((double)grid.cell[axis] * spacing - origin[axis]);
	}
	grid.corner = (voe_math_float3){ corner[0], corner[1], corner[2] };
	return grid;
}
