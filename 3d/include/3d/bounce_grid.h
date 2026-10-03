// The probe volume fitted to the eye, on the CPU (ADR-0326 point 2, 0329 point
// 1): which whole cells of the world the target's probe grid covers, and where
// its lowest corner sits about the eye. Arithmetic only; no device, no
// allocation.
//
//     voe_3d_bounce_grid grid = voe_3d_bounce_grid_fit(eye);
//     bounce.cell = grid.cell, bounce.corner = grid.corner ...
//     voe_render_bounce_begin(device, target, &bounce);
//
// THE GRID STANDS ON THE EYE, IN WHOLE CELLS ABOUT THE WORLD ORIGIN. The eye's
// cell is its position floored, in double, at VOE_RENDER_BOUNCE_SPACING; `cell`
// is that cell less half of VOE_RENDER_BOUNCE_PROBES_XZ (12) along x and z and
// less VOE_3D_BOUNCE_BELOW along y, so the grid moves a whole cell at a time and
// a probe always stands at the same world place. `corner` is that lowest cell's
// lowest corner about the eye, a small float.
//
// THE EYE AND NOT THE VIEW (0328, 0329): a volume placed by where the camera
// looks moves when it turns, empties probes and fades the room. Placed by the
// eye alone, turning in place never moves it.
//
// EIGHT CELLS BELOW, FOUR ABOVE: a camera 10.9 m up keeps the ground out of the
// faded outer cell, and a standing eye still keeps a room's ceiling.
#pragma once

#include <math/double3.h>
#include <math/float3.h>
#include <render/device.h>

#include <stdint.h>

// The volume's cells below the eye's cell (ADR-0329 point 1).
#define VOE_3D_BOUNCE_BELOW 8

typedef struct voe_3d_bounce_grid {
	int32_t cell[3];
	voe_math_float3 corner;
} voe_3d_bounce_grid;

voe_3d_bounce_grid voe_3d_bounce_grid_fit(voe_math_double3 eye);
