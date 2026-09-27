// The arithmetic every editor marker shares: segments into line quads about
// the eye, and a ray against a box in a marker's own space. Internal to 3d:
// camera_marker.c, collider_marker.c and sun_marker.c build their segments and
// hand them here.
//
//     voe_3d_marker_lines(segments, count, matrix, view, size, 2.0f, arena,
//                         &mesh);
//     if (voe_3d_marker_box_hit(about_ray, half, ray.direction, &distance))
//             ... // the marker is under the pointer
//
// THE QUADS ARE 3d/outline.h's, centred on the segment: a fixed pixel width
// from each end's own depth, extended half a width past both ends so corners
// have no notch, wound to face the eye with normals pointing at it, in the
// caller's arena (rule 11). The arithmetic mirrors outline.c's, which keeps its
// helpers to itself. A segment seen exactly end-on, or of no length, is left
// out, so the mesh may hold fewer quads than segments.
#pragma once

#include <3d/outline.h>

#include <base/arena.h>

#include <math/float3.h>
#include <math/float4x4.h>

#include <render/device.h>

#include <stdbool.h>
#include <stdint.h>

// One line of a marker, from `a` to `b`, in the marker's own space.
typedef struct {
	voe_math_float3 a;
	voe_math_float3 b;
} voe_3d_marker_segment;

// The quads of `count` segments under `matrix`, a marker's matrix about the
// eye `view` is about (ADR-0250), `pixels` wide on a picture `size` pixels big,
// pushed into `arena` and handed back in `out`. `size` must have area.
void voe_3d_marker_lines(const voe_3d_marker_segment *segments, uint32_t count,
			 voe_math_float4x4 matrix, voe_render_view view,
			 voe_platform_size size, float pixels,
			 voe_base_arena *arena, voe_3d_outline_mesh *out);

// Whether a ray from the origin along `direction` meets the box of `half`
// extents about a marker's origin, where `matrix` is the marker's matrix about
// the ray's own origin and has an inverse. The direction is taken into the
// marker's space and not renormalised, so `distance` (may be NULL) is along the
// world ray: where it enters the box, or leaves it when it starts inside.
[[nodiscard]] bool voe_3d_marker_box_hit(voe_math_float4x4 matrix,
					 voe_math_float3 half,
					 voe_math_float3 direction,
					 float *distance);
