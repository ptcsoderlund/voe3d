// The point light marker's three circles in world axes about its position,
// handed to marker_lines.c for their quads, and its cube's slab test — see the
// header for the geometry, why world axes and why the cube is hit.
#include <3d/point_light_marker.h>

#include "marker_lines.h"

#include <base/assert.h>

#include <math/float3.h>
#include <math/float4x4.h>

#include <scene/transform_component.h>

#include <math.h>
#include <stddef.h>

// The circles' radius and the cube's half extent, metres (0320 point 7).
#define RADIUS 0.25f
#define HALF 0.25f
#define PI 3.14159265358979f

typedef voe_3d_marker_segment segment;

// A pose at `position` in the world's axes, unscaled: all that places a lamp.
static voe_scene_transform placed(voe_math_double3 position)
{
	return (voe_scene_transform){
		.position = position,
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 1.0f, 1.0f, 1.0f },
	};
}

// One point of the circle in `plane` (0 XY, 1 YZ, 2 ZX) at `angle`: the cosine
// along the plane's first axis, the sine along its second.
static voe_math_float3 on_circle(uint32_t plane, float angle)
{
	float c = RADIUS * cosf(angle);
	float s = RADIUS * sinf(angle);

	if (plane == 0)
		return (voe_math_float3){ c, s, 0.0f };
	if (plane == 1)
		return (voe_math_float3){ 0.0f, c, s };
	return (voe_math_float3){ s, 0.0f, c };
}

// The marker's edges about the lamp: the XY circle, then YZ, then ZX.
static void marker_edges(segment edges[VOE_3D_POINT_LIGHT_MARKER_EDGES])
{
	uint32_t n = 0;

	for (uint32_t plane = 0; plane < 3; plane++) {
		for (uint32_t i = 0; i < VOE_3D_POINT_LIGHT_MARKER_CIRCLE; i++) {
			float from = 2.0f * PI * (float)i /
				     VOE_3D_POINT_LIGHT_MARKER_CIRCLE;
			float to = 2.0f * PI * (float)(i + 1) /
				   VOE_3D_POINT_LIGHT_MARKER_CIRCLE;

			edges[n++] = (segment){ on_circle(plane, from),
						on_circle(plane, to) };
		}
	}
	VOE_BASE_ASSERT(n == VOE_3D_POINT_LIGHT_MARKER_EDGES,
			"the point light marker's edge count drifted");
}

bool voe_3d_point_light_marker_quads(voe_math_double3 position,
				     voe_render_view view, voe_math_double3 eye,
				     voe_platform_size size, float pixels,
				     voe_base_arena *arena,
				     voe_3d_outline_mesh *out)
{
	segment edges[VOE_3D_POINT_LIGHT_MARKER_EDGES];

	VOE_BASE_ASSERT(arena != NULL, "a point light marker with no arena");
	VOE_BASE_ASSERT(out != NULL, "a point light marker into nothing");

	if (size.width == 0 || size.height == 0)
		return false;

	marker_edges(edges);
	// About the eye, as `view` is: the quads are eye-relative (ADR-0250).
	voe_3d_marker_lines(edges, VOE_3D_POINT_LIGHT_MARKER_EDGES,
			    voe_scene_transform_matrix(placed(position), eye),
			    view, size, pixels, arena, out);
	return true;
}

bool voe_3d_point_light_marker_hit(voe_math_double3 position, voe_3d_ray ray,
				   float *distance)
{
	VOE_BASE_ASSERT(isfinite(ray.direction.x) && isfinite(ray.direction.y) &&
				isfinite(ray.direction.z),
			"a point light hit along no direction");

	// About the ray's own origin, so only the small difference is narrowed
	// (ADR-0250).
	return voe_3d_marker_box_hit(
		voe_scene_transform_matrix(placed(position), ray.origin),
		(voe_math_float3){ HALF, HALF, HALF }, ray.direction, distance);
}
