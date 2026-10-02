// The probe volume fitted to one view, on the CPU (ADR-0326 point 2): which
// whole cells of the world the target's probe grid covers, and where its lowest
// corner sits about the eye. Arithmetic only; no device, no allocation.
//
//     voe_3d_bounce_grid grid = voe_3d_bounce_grid_fit(view, eye);
//     bounce.cell = grid.cell, bounce.corner = grid.corner ...
//     voe_render_bounce_begin(device, target, &bounce);
//
// THE GRID STANDS VOE_3D_BOUNCE_AHEAD METRES IN FRONT OF THE EYE, IN WHOLE CELLS
// ABOUT THE WORLD ORIGIN. Its centre is eye + forward × ahead, taken in double;
// `cell` is that centre's cell at VOE_RENDER_BOUNCE_SPACING less half of
// VOE_RENDER_BOUNCE_PROBES_XZ along x and z and half of
// VOE_RENDER_BOUNCE_PROBES_Y along y (12, 6 and 12 cells), so the grid moves a
// whole cell at a time and a probe always stands at the same world place.
// `corner` is that lowest cell's lowest corner about the eye, a small float.
//
// `view` IS EYE-RELATIVE (0250). Forward is read out of its rotation, its -Z
// row; a view with no forward asserts.
#pragma once

#include <math/double3.h>
#include <math/float3.h>
#include <render/device.h>

#include <stdint.h>

// Metres ahead of the eye the probe volume centres (ADR-0326 point 2).
#define VOE_3D_BOUNCE_AHEAD 24.0f

typedef struct voe_3d_bounce_grid {
	int32_t cell[3];
	voe_math_float3 corner;
} voe_3d_bounce_grid;

voe_3d_bounce_grid voe_3d_bounce_grid_fit(voe_render_view view,
					  voe_math_double3 eye);
