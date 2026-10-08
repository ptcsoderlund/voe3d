// The fit of the probe volume to the still casters' box: the smallest
// power-of-two spacing whose grid, its lowest cell the box centre's cell less
// (12, 6, 12), holds the box's cells with one spare, in double about the world
// origin, and its corner about the eye; a nest's cell about the eye, held
// within 2 cells; and the relight's sun view of either volume through
// light_box.h. See 3d/bounce_grid.h for why each step is there.
#include "light_box.h"

#include <3d/bounce_grid.h>
#include <base/assert.h>

#include <math.h>
#include <stdbool.h>

// The largest k of the spacing VOE_RENDER_BOUNCE_SPACING × 2^k (0332 point 2).
#define COARSEST 16

// Whether the grid at `spacing` about the centre `centre` holds `min` to `max`
// with a cell to spare each side; its lowest cell into `cell`.
static bool holds(const double min[3], const double max[3],
		  const double centre[3], double spacing, int32_t cell[3])
{
	const int32_t below[3] = { VOE_RENDER_BOUNCE_PROBES_XZ / 2,
				   VOE_RENDER_BOUNCE_PROBES_Y / 2,
				   VOE_RENDER_BOUNCE_PROBES_XZ / 2 };
	const int32_t count[3] = { VOE_RENDER_BOUNCE_PROBES_XZ,
				   VOE_RENDER_BOUNCE_PROBES_Y,
				   VOE_RENDER_BOUNCE_PROBES_XZ };
	bool held = true;

	for (int axis = 0; axis < 3; axis++) {
		cell[axis] = (int32_t)floor(centre[axis] / spacing) - below[axis];
		held = held && floor(min[axis] / spacing) >= (double)cell[axis] + 1 &&
		       floor(max[axis] / spacing) <=
			       (double)cell[axis] + count[axis] - 2;
	}
	return held;
}

voe_3d_bounce_grid voe_3d_bounce_grid_fit(voe_math_double3 min,
					  voe_math_double3 max,
					  voe_math_double3 eye)
{
	voe_3d_bounce_grid grid = { 0 };
	const double low[3] = { min.x, min.y, min.z };
	const double high[3] = { max.x, max.y, max.z };
	const double origin[3] = { eye.x, eye.y, eye.z };
	bool boxed = min.x <= max.x && min.y <= max.y && min.z <= max.z;
	double centre[3] = { 0.0, 0.0, 0.0 };
	double spacing = (double)VOE_RENDER_BOUNCE_SPACING;
	float corner[3];

	VOE_BASE_ASSERT(isfinite(eye.x) && isfinite(eye.y) && isfinite(eye.z),
			"an eye that is not a number");
	VOE_BASE_ASSERT(!boxed || (isfinite(min.x) && isfinite(min.y) &&
				   isfinite(min.z) && isfinite(max.x) &&
				   isfinite(max.y) && isfinite(max.z)),
			"a box that is not a number");
	for (int axis = 0; boxed && axis < 3; axis++)
		centre[axis] = 0.5 * (low[axis] + high[axis]);
	// No box is the origin alone, which every grid about it holds at k 0.
	for (int k = 0; k <= COARSEST; k++) {
		spacing = (double)VOE_RENDER_BOUNCE_SPACING * ldexp(1.0, k);
		if (holds(boxed ? low : centre, boxed ? high : centre, centre,
			  spacing, grid.cell))
			break;
	}
	for (int axis = 0; axis < 3; axis++)
		corner[axis] = (float)((double)grid.cell[axis] * spacing - origin[axis]);
	grid.spacing = (float)spacing;
	grid.corner = (voe_math_float3){ corner[0], corner[1], corner[2] };
	VOE_BASE_ASSERT(isfinite(grid.corner.x) && isfinite(grid.corner.y) &&
				isfinite(grid.corner.z),
			"a corner that is not a number");
	return grid;
}

float voe_3d_bounce_nest_spacing(uint32_t nest)
{
	VOE_BASE_ASSERT(nest < VOE_3D_BOUNCE_NESTS, "a nest past the last");
	return ldexpf(16.0f, -2 * (int)nest);
}

// How far the nest at `placed` moves on one axis: by as much as brings `home`,
// the eye's own home cell, within 2 cells of it, else not at all.
static int32_t nest_move(int32_t placed, int32_t home)
{
	if (home - placed > 2)
		return home - placed - 2;
	if (home - placed < -2)
		return home - placed + 2;
	return 0;
}

voe_3d_bounce_grid voe_3d_bounce_grid_nest(float spacing, const int32_t *placed,
					   voe_math_double3 eye)
{
	const int32_t below[3] = { VOE_RENDER_BOUNCE_PROBES_XZ / 2, 9,
				   VOE_RENDER_BOUNCE_PROBES_XZ / 2 };
	const int32_t count[3] = { VOE_RENDER_BOUNCE_PROBES_XZ,
				   VOE_RENDER_BOUNCE_PROBES_Y,
				   VOE_RENDER_BOUNCE_PROBES_XZ };
	const double at[3] = { eye.x, eye.y, eye.z };
	voe_3d_bounce_grid grid = { .spacing = spacing };
	int32_t home[3];
	bool kept = placed != NULL;
	float corner[3];

	VOE_BASE_ASSERT(isfinite(spacing) && spacing > 0.0f,
			"a nest with no spacing");
	VOE_BASE_ASSERT(isfinite(eye.x) && isfinite(eye.y) && isfinite(eye.z),
			"an eye that is not a number");
	for (int axis = 0; axis < 3; axis++) {
		home[axis] = (int32_t)floor(at[axis] / (double)spacing) - below[axis];
		if (kept) {
			int32_t move = nest_move(placed[axis], home[axis]);

			grid.cell[axis] = placed[axis] + move;
			kept = move < count[axis] && move > -count[axis];
		}
	}
	for (int axis = 0; axis < 3; axis++) {
		if (!kept)
			grid.cell[axis] = home[axis];
		corner[axis] = (float)((double)grid.cell[axis] * (double)spacing -
				       at[axis]);
	}
	grid.corner = (voe_math_float3){ corner[0], corner[1], corner[2] };
	return grid;
}

// A sphere about the volume's centre reaching every surface a probe sees,
// snapped to whole texels and looked at from the sun as a cascade is.
voe_render_view voe_3d_bounce_grid_sun(voe_3d_bounce_grid grid,
				       voe_math_double3 eye,
				       voe_math_float3 direction)
{
	voe_math_float3 half_sides = {
		0.5f * (float)VOE_RENDER_BOUNCE_PROBES_XZ * grid.spacing,
		0.5f * (float)VOE_RENDER_BOUNCE_PROBES_Y * grid.spacing,
		0.5f * (float)VOE_RENDER_BOUNCE_PROBES_XZ * grid.spacing
	};
	float reach = VOE_RENDER_BOUNCE_REACH * grid.spacing /
		      VOE_RENDER_BOUNCE_SPACING;
	float radius = voe_math_float3_length(half_sides) + reach;
	double texel = 2.0 * (double)radius / (double)VOE_RENDER_BOUNCE_SHADOW_TEXELS;
	struct voe_3d_light_basis basis;
	voe_math_float3 centre;

	VOE_BASE_ASSERT(grid.spacing > 0.0f && isfinite(grid.spacing),
			"a grid with no spacing");
	VOE_BASE_ASSERT(fabsf(voe_math_float3_length(direction) - 1.0f) < 1e-3f,
			"a sun whose direction is not unit length");
	basis = voe_3d_light_box_basis(direction);
	centre = voe_3d_light_box_snap(voe_math_float3_add(grid.corner, half_sides),
				       eye, basis, texel);
	return voe_3d_light_box_look(centre, radius, radius, basis);
}
