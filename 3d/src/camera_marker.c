// The camera marker's edges, handed to marker_lines.c for their quads and its
// box's slab test — see the header for the geometry, why the quads are
// outline.h's and why only the box is hit.
#include <3d/camera_marker.h>

#include "marker_lines.h"

#include <base/assert.h>

#include <math/float3.h>
#include <math/float4x4.h>

#include <math.h>
#include <stdbool.h>
#include <stddef.h>

// The box's half extents in the camera's own space, metres (0223).
#define HALF_X 0.1f
#define HALF_Y 0.075f
#define HALF_Z 0.15f

// How far ahead the frustum reaches, metres, and its width over its height.
#define REACH 1.0f
#define ASPECT (16.0f / 9.0f)

typedef voe_3d_marker_segment segment;

// The marker's twenty edges in the camera's own space. Box corner i has its
// x, y and z signs from bits 0, 1 and 2; an edge joins two corners one bit
// apart.
static void marker_edges(voe_scene_camera lens,
			 segment edges[VOE_3D_CAMERA_MARKER_EDGES])
{
	voe_math_float3 box[8];
	voe_math_float3 rim[4];
	float up = tanf(lens.fov_y * 0.5f) * REACH;
	float across = up * ASPECT;
	uint32_t n = 0;

	for (uint32_t i = 0; i < 8; i++)
		box[i] = (voe_math_float3){ (i & 1) ? HALF_X : -HALF_X,
					    (i & 2) ? HALF_Y : -HALF_Y,
					    (i & 4) ? HALF_Z : -HALF_Z };
	for (uint32_t i = 0; i < 8; i++)
		for (uint32_t bit = 1; bit < 8; bit <<= 1)
			if ((i & bit) == 0)
				edges[n++] = (segment){ box[i], box[i | bit] };

	// Around the far rectangle, so corner k and k + 1 are neighbours.
	rim[0] = (voe_math_float3){ -across, -up, -REACH };
	rim[1] = (voe_math_float3){ across, -up, -REACH };
	rim[2] = (voe_math_float3){ across, up, -REACH };
	rim[3] = (voe_math_float3){ -across, up, -REACH };
	for (uint32_t k = 0; k < 4; k++) {
		edges[n++] = (segment){ (voe_math_float3){ 0.0f, 0.0f, 0.0f },
					rim[k] };
		edges[n++] = (segment){ rim[k], rim[(k + 1) % 4] };
	}
	VOE_BASE_ASSERT(n == VOE_3D_CAMERA_MARKER_EDGES,
			"the marker's edge count drifted");
}

bool voe_3d_camera_marker_quads(voe_scene_transform pose,
				voe_scene_camera lens, voe_render_view view,
				voe_math_double3 eye, voe_platform_size size,
				float pixels,
				voe_base_arena *arena,
				voe_3d_outline_mesh *out)
{
	segment edges[VOE_3D_CAMERA_MARKER_EDGES];
	// About the eye, as `view` is: the quads are eye-relative (ADR-0250).
	voe_math_float4x4 matrix = voe_scene_transform_matrix(pose, eye);

	VOE_BASE_ASSERT(arena != NULL, "a camera marker with no arena");
	VOE_BASE_ASSERT(out != NULL, "a camera marker into nothing");

	if (voe_math_float4x4_determinant(matrix) == 0.0f ||
	    size.width == 0 || size.height == 0)
		return false;

	marker_edges(lens, edges);
	voe_3d_marker_lines(edges, VOE_3D_CAMERA_MARKER_EDGES, matrix, view,
			    size, pixels, arena, out);
	return true;
}

bool voe_3d_camera_marker_hit(voe_scene_transform pose, voe_3d_ray ray,
			      float *distance)
{
	// About the ray's own origin, so the ray starts at nought in float and
	// only the small difference is narrowed (ADR-0250).
	voe_math_float4x4 matrix = voe_scene_transform_matrix(pose, ray.origin);

	if (voe_math_float4x4_determinant(matrix) == 0.0f)
		return false;
	return voe_3d_marker_box_hit(
		matrix, (voe_math_float3){ HALF_X, HALF_Y, HALF_Z },
		ray.direction, distance);
}
