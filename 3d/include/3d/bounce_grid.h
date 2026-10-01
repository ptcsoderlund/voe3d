// The sun's bounce grid fitted to one view, on the CPU (ADR-0308): which whole
// cells of the world the target's probe grid covers, where its lowest corner
// sits about the eye, and the light view the bounce pass draws with.
// Arithmetic only; no device, no allocation.
//
//     voe_3d_bounce_grid grid = voe_3d_bounce_grid_fit(view, eye, sun);
//     voe_render_bounce_pass_begin(device, grid.light, ...);
//     update.cell = grid.cell, update.corner = grid.corner ...
//
// THE GRID STANDS VOE_3D_BOUNCE_AHEAD METRES IN FRONT OF THE EYE, IN WHOLE CELLS
// ABOUT THE WORLD ORIGIN. Its centre is eye + forward × ahead, taken in double;
// `cell` is that centre's cell at VOE_RENDER_BOUNCE_SPACING less half of
// VOE_RENDER_BOUNCE_PROBES on each axis, so the grid moves a whole cell at a
// time and a probe always stands at the same world place. `corner` is that
// lowest cell's corner about the eye, a small float.
//
// THE LIGHT VIEW LOOKS FROM THE SUN AT THE GRID'S CENTRE (light_box.h, shared
// with the cascades): an orthographic box about the grid's bounding sphere,
// radius √3 × probes × spacing / 2 rounded up to a centimetre, reaching
// VOE_3D_SHADOW_CASTER_REACH toward the sun. Its centre snaps, in double, to
// whole 8-texel blocks of VOE_RENDER_BOUNCE_TEXELS — the blocks the update
// reduces to one light each — so a moving eye moves the map under a fixed
// world. The box's half side is radius × texels / (texels − 8): exactly enough
// that the snapped box still holds the sphere.
//
// EVERY MATRIX IS EYE-RELATIVE like `view` (0250). Forward is read out of
// `view`'s rotation, its -Z row. `direction` is where the light goes, unit
// length, as voe_render_light's; one that is not asserts.
#pragma once

#include <math/double3.h>
#include <math/float3.h>
#include <render/device.h>

#include <stdint.h>

// Metres ahead of the eye the bounce grid centres.
#define VOE_3D_BOUNCE_AHEAD 32.0f

typedef struct voe_3d_bounce_grid {
	int32_t cell[3];
	voe_math_float3 corner;
	voe_render_view light;
} voe_3d_bounce_grid;

voe_3d_bounce_grid voe_3d_bounce_grid_fit(voe_render_view view,
					  voe_math_double3 eye,
					  voe_math_float3 direction);
