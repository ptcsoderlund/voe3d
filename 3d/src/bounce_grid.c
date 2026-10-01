// The fit of the bounce grid to one view: forward out of the view, the grid's
// whole cells about the world origin in double, its corner about the eye, and
// the sun's view of its bounding sphere snapped to whole 8-texel blocks. See
// 3d/bounce_grid.h for why each step is there; the basis, snap and look are
// light_box.h's.
#include "light_box.h"

#include <3d/bounce_grid.h>
#include <base/assert.h>

#include <math.h>

// Texels a side of one block the update reduces to a single light.
#define BLOCK_TEXELS 8

voe_3d_bounce_grid voe_3d_bounce_grid_fit(voe_render_view view,
					  voe_math_double3 eye,
					  voe_math_float3 direction)
{
	voe_3d_bounce_grid grid = { 0 };
	voe_math_float3 forward = voe_math_float3_normalize((voe_math_float3){
		-view.view.m[2][0], -view.view.m[2][1], -view.view.m[2][2] });
	double spacing = (double)VOE_RENDER_BOUNCE_SPACING;
	double ahead = (double)VOE_3D_BOUNCE_AHEAD;
	double centre[3] = { eye.x + forward.x * ahead, eye.y + forward.y * ahead,
			     eye.z + forward.z * ahead };
	double origin[3] = { eye.x, eye.y, eye.z };
	float corner[3];
	float side = (float)VOE_RENDER_BOUNCE_PROBES * VOE_RENDER_BOUNCE_SPACING;
	float radius = (float)(ceil(sqrt(3.0) * (double)side * 0.5 * 100.0) / 100.0);
	float half = radius * (float)VOE_RENDER_BOUNCE_TEXELS /
		     (float)(VOE_RENDER_BOUNCE_TEXELS - BLOCK_TEXELS);
	double block = 2.0 * (double)half * BLOCK_TEXELS / VOE_RENDER_BOUNCE_TEXELS;
	struct voe_3d_light_basis basis;
	voe_math_float3 middle;

	VOE_BASE_ASSERT(isfinite(forward.x) && isfinite(forward.y) &&
				isfinite(forward.z),
			"a view with no forward");
	VOE_BASE_ASSERT(fabsf(voe_math_float3_length(direction) - 1.0f) < 1e-3f,
			"a sun whose direction is not unit length");

	for (int axis = 0; axis < 3; axis++) {
		grid.cell[axis] = (int32_t)floor(centre[axis] / spacing) -
				  VOE_RENDER_BOUNCE_PROBES / 2;
		corner[axis] = (float)((double)grid.cell[axis] * spacing - origin[axis]);
	}
	grid.corner = (voe_math_float3){ corner[0], corner[1], corner[2] };
	middle = voe_math_float3_add(grid.corner, (voe_math_float3){
		side * 0.5f, side * 0.5f, side * 0.5f });

	basis = voe_3d_light_box_basis(direction);
	grid.light = voe_3d_light_box_look(
		voe_3d_light_box_snap(middle, eye, basis, block), half, radius, basis);
	VOE_BASE_ASSERT(half > radius, "a light box smaller than the grid");
	return grid;
}
