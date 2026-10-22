// Whether a sphere about the eye is inside a view, internal to 3d: the one place
// the folder asks it (ADR-0464 point 1).
//
//     struct voe_3d_cull cull = voe_3d_cull_of(frame_view, false);
//     if (voe_3d_cull_sees(&cull, centre, voe_3d_cull_radius(world, r)))
//             draw(...);
//
// A SPHERE, BECAUSE IT IS CONSERVATIVE. A sphere wholly outside one plane is
// outside the view; anything else is drawn, so nothing visible is ever skipped
// and nothing pops at an edge. A sphere near a corner of the volume is kept
// though it lies outside: drawn for nothing, never missing.
//
// THE PLANES ARE EYE-RELATIVE (ADR-0250), because the view is: its view matrix
// has no translation and its `eye` is zero, so a centre is handed in as the
// drawable's world place minus the eye's, in float. Taken from projection ×
// view by this engine's clip space (3d/include/3d/projection.h): x and y in
// ±w, depth w at the near plane and 0 at the far one, no Y flip. Each plane is
// normalised, xyz the inward normal and w the offset, so a distance is metres.
//
// A CASCADE LEAVES ITS SUN SIDE OPEN (ADR-0464 point 3). `sun_side_open` drops
// the near plane, so a caster further toward the sun than a cascade's box
// (`voe_3d_shadow_cascades_fit`'s `light[c]`) still casts into it, as it did
// before anything was culled. The camera keeps all six.
//
// IT IS 3d'S AND NOT render'S (ADR-0464). Render cannot see a landscape node's
// heights or a water's waves, displaced in the vertex stage; 3d knows every
// drawable's true size, so the sphere is built here.
#pragma once

#include <math/float3.h>
#include <math/float4.h>
#include <math/float4x4.h>
#include <render/device.h>

#include <stdint.h>

struct voe_3d_cull {
	voe_math_float4 planes[6];
	uint32_t count;
};

// The planes of `view`: left, right, bottom, top, then near unless
// `sun_side_open`, then far.
struct voe_3d_cull voe_3d_cull_of(voe_render_view view, bool sun_side_open);

// False only when the sphere at eye-relative `centre` lies wholly outside some
// plane.
bool voe_3d_cull_sees(const struct voe_3d_cull *cull, voe_math_float3 centre,
		      float radius);

// `radius` times the longest of `world`'s upper 3×3 columns: a sphere's radius
// under a world matrix, whatever its rotation or uneven scale.
float voe_3d_cull_radius(voe_math_float4x4 world, float radius);
