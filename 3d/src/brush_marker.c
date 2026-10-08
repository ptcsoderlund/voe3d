// The brush's two rings in the grid's own space, each point on the ground plus
// 5 cm, handed to marker_lines.c for their quads — see the header for why on
// the ground and why two rings.
#include <3d/brush_marker.h>

#include <3d/landscape.h>

#include "marker_lines.h"

#include <base/assert.h>

#include <math.h>
#include <stddef.h>

// How far above the ground a ring point sits, metres in the grid's space.
#define LIFT 0.05f
#define PI 3.14159265358979f

// The point of a ring of `radius` about (x, z) at `i` segments round.
static voe_math_float3 ring_point(const voe_assets_landscape *landscape,
				  float x, float z, float radius, uint32_t i)
{
	float angle = 2.0f * PI * (float)i / VOE_3D_BRUSH_MARKER_SEGMENTS;
	float px = x + radius * cosf(angle);
	float pz = z + radius * sinf(angle);

	return (voe_math_float3){
		px, voe_3d_landscape_height(landscape, px, pz) + LIFT, pz
	};
}

bool voe_3d_brush_marker_quads(const voe_assets_landscape *landscape,
			       voe_scene_transform pose, float x, float z,
			       float radius, float inner, voe_render_view view,
			       voe_math_double3 eye, voe_platform_size size,
			       float pixels, voe_base_arena *arena,
			       voe_3d_outline_mesh *out)
{
	const float radii[2] = { radius, inner };
	voe_3d_marker_segment edges[VOE_3D_BRUSH_MARKER_EDGES];
	uint32_t n = 0;

	VOE_BASE_ASSERT(landscape != NULL, "a brush marker on no landscape");
	VOE_BASE_ASSERT(arena != NULL, "a brush marker with no arena");
	VOE_BASE_ASSERT(out != NULL, "a brush marker into nothing");

	if (size.width == 0 || size.height == 0)
		return false;

	for (uint32_t ring = 0; ring < 2; ring++)
		for (uint32_t i = 0; i < VOE_3D_BRUSH_MARKER_SEGMENTS; i++)
			edges[n++] = (voe_3d_marker_segment){
				ring_point(landscape, x, z, radii[ring], i),
				ring_point(landscape, x, z, radii[ring], i + 1),
			};
	voe_3d_marker_lines(edges, VOE_3D_BRUSH_MARKER_EDGES,
			    voe_scene_transform_matrix(pose, eye), view, size,
			    pixels, arena, out);
	return true;
}
