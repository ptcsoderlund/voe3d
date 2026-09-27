// The sun marker's edges in its own space, handed to marker_lines.c for their
// quads and its cube's slab test — see the header for the geometry, why scale
// is ignored and why the cube is hit.
#include <3d/sun_marker.h>

#include "marker_lines.h"

#include <base/assert.h>

#include <math/float3.h>
#include <math/float4x4.h>

#include <math.h>
#include <stddef.h>

// The circle's radius, the arrow's length and the cube's half extent, metres
// (0274); the head's length back from the tip and its half width.
#define RADIUS 0.25f
#define REACH 1.0f
#define HALF 0.25f
#define HEAD_LENGTH 0.15f
#define HEAD_HALF 0.075f
#define PI 3.14159265358979f

typedef voe_3d_marker_segment segment;

// The pose with its scale taken off, so only position and rotation place it.
static voe_scene_transform unscaled(voe_scene_transform pose)
{
	pose.scale = (voe_math_float3){ 1.0f, 1.0f, 1.0f };
	return pose;
}

// The marker's edges in the sun's own space: the circle, then the shaft from
// the centre to the tip, then the head from the tip back towards +X, -X, +Y
// and -Y.
static void marker_edges(segment edges[VOE_3D_SUN_MARKER_EDGES])
{
	voe_math_float3 tip = { 0.0f, 0.0f, -REACH };
	float back = -REACH + HEAD_LENGTH;
	uint32_t n = 0;

	for (uint32_t i = 0; i < VOE_3D_SUN_MARKER_CIRCLE; i++) {
		float from = 2.0f * PI * (float)i / VOE_3D_SUN_MARKER_CIRCLE;
		float to = 2.0f * PI * (float)(i + 1) / VOE_3D_SUN_MARKER_CIRCLE;

		edges[n++] = (segment){
			{ RADIUS * cosf(from), RADIUS * sinf(from), 0.0f },
			{ RADIUS * cosf(to), RADIUS * sinf(to), 0.0f },
		};
	}
	edges[n++] = (segment){ { 0.0f, 0.0f, 0.0f }, tip };
	edges[n++] = (segment){ tip, { HEAD_HALF, 0.0f, back } };
	edges[n++] = (segment){ tip, { -HEAD_HALF, 0.0f, back } };
	edges[n++] = (segment){ tip, { 0.0f, HEAD_HALF, back } };
	edges[n++] = (segment){ tip, { 0.0f, -HEAD_HALF, back } };
	VOE_BASE_ASSERT(n == VOE_3D_SUN_MARKER_EDGES,
			"the sun marker's edge count drifted");
}

bool voe_3d_sun_marker_quads(voe_scene_transform pose, voe_render_view view,
			     voe_math_double3 eye, voe_platform_size size,
			     float pixels, voe_base_arena *arena,
			     voe_3d_outline_mesh *out)
{
	segment edges[VOE_3D_SUN_MARKER_EDGES];

	VOE_BASE_ASSERT(arena != NULL, "a sun marker with no arena");
	VOE_BASE_ASSERT(out != NULL, "a sun marker into nothing");

	if (size.width == 0 || size.height == 0)
		return false;

	marker_edges(edges);
	// About the eye, as `view` is: the quads are eye-relative (ADR-0250).
	voe_3d_marker_lines(edges, VOE_3D_SUN_MARKER_EDGES,
			    voe_scene_transform_matrix(unscaled(pose), eye),
			    view, size, pixels, arena, out);
	return true;
}

bool voe_3d_sun_marker_hit(voe_scene_transform pose, voe_3d_ray ray,
			   float *distance)
{
	// About the ray's own origin, so only the small difference is narrowed
	// (ADR-0250); unscaled, so it always has an inverse.
	return voe_3d_marker_box_hit(
		voe_scene_transform_matrix(unscaled(pose), ray.origin),
		(voe_math_float3){ HALF, HALF, HALF }, ray.direction, distance);
}
