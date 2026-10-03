// The probe volume fitted to the level, on the CPU (ADR-0331, 0332 points 2
// and 4): the spacing and the whole cells of the world the target's probe grid
// covers, fitted to the still casters' box, and where its lowest corner sits
// about the eye; and the light view of the relight's sun map that covers it.
// Arithmetic only; no device, no allocation.
//
//     voe_3d_bounce_grid grid = voe_3d_bounce_grid_fit(min, max, eye);
//     bounce.spacing = grid.spacing, bounce.cell = grid.cell ...
//     voe_render_bounce_begin(device, target, &bounce);
//     light = voe_3d_bounce_grid_sun(grid, eye, sun.direction);
//
// ON THE LEVEL, NOT THE EYE (0331): the same world gives the same grid in every
// view, so a camera that moves or turns never moves it, never recaptures a
// probe and never changes what a far thing gets. The eye is used only for
// `corner`, the lowest cell's lowest corner about the eye, a small float.
//
// THE FIT: the spacing is VOE_RENDER_BOUNCE_SPACING × 2^k, k from 0 to at most
// 16, the smallest at which the grid whose lowest cell is the box centre's cell
// less (12, 6, 12) holds every cell the box touches with one cell to spare on
// each side. Cells are whole cells of that spacing about the world origin, in
// double, so a probe stands at the same world place whatever the eye; a box
// whose centre crosses a cell edge moves `cell` by one. A `min` above `max` on
// any axis is no still caster: k 0 about the world origin.
//
// WHY POWERS OF TWO: a new spacing empties and recaptures all 6912 probes, so
// a box that grows a little while editing must not change it; doubling is
// rare. One spacing for all three axes, because the shaders take one.
//
// WHY A CELL SPARE: the read fades over the grid's outer cell, so the box kept
// out of it keeps that fade outside the level.
//
// THE RELIGHT'S SUN MAP IS FITTED TO THE VOLUME (0329 point 2), not the view:
// voe_3d_bounce_grid_sun is an orthographic light view of a sphere about the
// volume's centre of its half-diagonal plus the volume's reach,
// VOE_RENDER_BOUNCE_REACH × spacing / VOE_RENDER_BOUNCE_SPACING (0332 point 4),
// so it holds every surface a probe can see; VOE_RENDER_BOUNCE_SHADOW_TEXELS
// across 2 × radius, through light_box.h's basis, snap and look as a
// cascade's. The snap is in double about the world origin, so it does not
// depend on where the camera stands or looks.
//
// Constraints: past 2^16 × the finest spacing a box too big is held as far as
// that grid reaches, its outer parts unbounced.
#pragma once

#include <math/double3.h>
#include <math/float3.h>
#include <render/device.h>

#include <stdint.h>

typedef struct voe_3d_bounce_grid {
	float spacing;
	int32_t cell[3];
	voe_math_float3 corner;
} voe_3d_bounce_grid;

// The grid fitted to the box `min` to `max`, in double about the world origin,
// its corner about `eye`. A coordinate that is not a number asserts.
voe_3d_bounce_grid voe_3d_bounce_grid_fit(voe_math_double3 min,
					  voe_math_double3 max,
					  voe_math_double3 eye);

// The light view, eye-relative, of the relight's sun map for `grid` fitted
// about `eye`, the sun shining along unit `direction`. A direction not of unit
// length asserts.
voe_render_view voe_3d_bounce_grid_sun(voe_3d_bounce_grid grid,
				       voe_math_double3 eye,
				       voe_math_float3 direction);
