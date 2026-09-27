// A collider's lines: each kind's segments in the shape's own space, then
// their quads from marker_lines.c, as the camera marker's are — see the header
// for why lines and why the shape is physics's.
#include <3d/collider_marker.h>

#include "marker_lines.h"

#include <base/assert.h>

#include <math/float3.h>
#include <math/float4x4.h>

#include <physics/collider_component.h>

#include <math.h>
#include <stdbool.h>
#include <stddef.h>

#define CIRCLE VOE_3D_COLLIDER_MARKER_CIRCLE
#define PI 3.14159265358979f

typedef voe_3d_marker_segment segment;

static voe_math_float3 at(float x, float y, float z)
{
	return (voe_math_float3){ x, y, z };
}

// `count` segments of an arc about `centre` in the plane of the unit axes `u`
// and `v`, from angle 0 through `sweep` radians, appended at edges[*n].
static void arc(segment *edges, uint32_t *n, voe_math_float3 centre,
		voe_math_float3 u, voe_math_float3 v, float radius,
		float sweep, uint32_t count)
{
	voe_math_float3 last = voe_math_float3_add(
		centre, voe_math_float3_scale(u, radius));

	for (uint32_t i = 1; i <= count; i++) {
		float angle = sweep * (float)i / (float)count;
		voe_math_float3 next = voe_math_float3_add(
			centre,
			voe_math_float3_add(
				voe_math_float3_scale(u, radius * cosf(angle)),
				voe_math_float3_scale(v, radius * sinf(angle))));

		edges[(*n)++] = (segment){ last, next };
		last = next;
	}
}

// The segments of `shape` in its own space, unrotated and about its centre;
// how many there are, 0 for a kind physics does not define. Box corner i has
// its x, y and z signs from bits 0, 1 and 2; an edge joins two corners one bit
// apart.
static uint32_t shape_edges(voe_physics_shape shape,
			    segment edges[VOE_3D_COLLIDER_MARKER_EDGES])
{
	voe_math_float3 h = shape.half;
	voe_math_float3 o = at(0.0f, 0.0f, 0.0f);
	voe_math_float3 x = at(1.0f, 0.0f, 0.0f);
	voe_math_float3 y = at(0.0f, 1.0f, 0.0f);
	voe_math_float3 z = at(0.0f, 0.0f, 1.0f);
	uint32_t n = 0;

	if (shape.kind == VOE_PHYSICS_COLLIDER_BOX) {
		voe_math_float3 box[8];

		for (uint32_t i = 0; i < 8; i++)
			box[i] = at((i & 1) ? h.x : -h.x, (i & 2) ? h.y : -h.y,
				    (i & 4) ? h.z : -h.z);
		for (uint32_t i = 0; i < 8; i++)
			for (uint32_t bit = 1; bit < 8; bit <<= 1)
				if ((i & bit) == 0)
					edges[n++] =
						(segment){ box[i], box[i | bit] };
	} else if (shape.kind == VOE_PHYSICS_COLLIDER_SPHERE) {
		arc(edges, &n, o, x, y, h.x, 2.0f * PI, CIRCLE);
		arc(edges, &n, o, y, z, h.x, 2.0f * PI, CIRCLE);
		arc(edges, &n, o, z, x, h.x, 2.0f * PI, CIRCLE);
	} else if (shape.kind == VOE_PHYSICS_COLLIDER_CAPSULE) {
		// The upright part's half height: the whole half less a cap.
		float r = h.x;
		float up = h.y - r;
		voe_math_float3 top = at(0.0f, up, 0.0f);
		voe_math_float3 bottom = at(0.0f, -up, 0.0f);
		voe_math_float3 down = at(0.0f, -1.0f, 0.0f);

		arc(edges, &n, top, x, z, r, 2.0f * PI, CIRCLE);
		arc(edges, &n, bottom, x, z, r, 2.0f * PI, CIRCLE);
		edges[n++] = (segment){ at(r, -up, 0.0f), at(r, up, 0.0f) };
		edges[n++] = (segment){ at(-r, -up, 0.0f), at(-r, up, 0.0f) };
		edges[n++] = (segment){ at(0.0f, -up, r), at(0.0f, up, r) };
		edges[n++] = (segment){ at(0.0f, -up, -r), at(0.0f, up, -r) };
		arc(edges, &n, top, x, y, r, PI, CIRCLE / 2);
		arc(edges, &n, bottom, x, down, r, PI, CIRCLE / 2);
		arc(edges, &n, top, z, y, r, PI, CIRCLE / 2);
		arc(edges, &n, bottom, z, down, r, PI, CIRCLE / 2);
	}
	VOE_BASE_ASSERT(n <= VOE_3D_COLLIDER_MARKER_EDGES,
			"a collider's segments outgrew the marker's room");
	return n;
}

bool voe_3d_collider_marker_quads(voe_physics_shape shape,
				  voe_render_view view, voe_math_double3 eye,
				  voe_platform_size size, float pixels,
				  voe_base_arena *arena,
				  voe_3d_outline_mesh *out)
{
	segment edges[VOE_3D_COLLIDER_MARKER_EDGES];
	// About the eye, as `view` is: the centre is narrowed only once the
	// eye is taken off it in double (ADR-0250).
	voe_math_float4x4 matrix = voe_math_float4x4_mul(
		voe_math_float4x4_from_translation(voe_math_double3_to_float3(
			voe_math_double3_sub(shape.centre, eye))),
		voe_math_float4x4_from_quat(shape.rotation));
	uint32_t count;

	VOE_BASE_ASSERT(arena != NULL, "a collider marker with no arena");
	VOE_BASE_ASSERT(out != NULL, "a collider marker into nothing");

	if (size.width == 0 || size.height == 0)
		return false;
	count = shape_edges(shape, edges);
	if (count == 0)
		return false;

	voe_3d_marker_lines(edges, count, matrix, view, size, pixels, arena,
			    out);
	return true;
}
