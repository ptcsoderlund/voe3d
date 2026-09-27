// The markers' line quads and box slab test — see the header for the width,
// the winding and what is left out.
#include "marker_lines.h"

#include <base/assert.h>

#include <math.h>
#include <stddef.h>

// outline.c's: an edge whose cross product with the eye is below this is seen
// end-on and has no quad.
#define DEGENERATE 1e-12f

// Mirrors outline.c's: how far in front of the eye `p` is, in metres.
static float depth_of(voe_render_view view, voe_math_float3 p)
{
	return -(view.view.m[2][0] * p.x + view.view.m[2][1] * p.y +
		 view.view.m[2][2] * p.z + view.view.m[2][3]);
}

// Mirrors outline.c's: half the line's width in metres at `p`, so the line is
// `pixels` pixels across at any distance.
static float half_width(voe_render_view view, voe_platform_size size,
			float pixels, voe_math_float3 p)
{
	return depth_of(view, p) * pixels /
	       (view.projection.m[1][1] * (float)size.height);
}

// Mirrors outline.c's: one corner of a quad, its normal pointing at the eye.
static voe_render_vertex facing(voe_math_float3 position, voe_math_float3 eye)
{
	return (voe_render_vertex){
		.position = position,
		.normal = voe_math_float3_normalize(
			voe_math_float3_sub(eye, position)),
		.uv = { 0.0f, 0.0f },
	};
}

// Mirrors outline.c's: one triangle, its last two corners swapped when it
// would face away from the eye and be culled.
static void triangle(uint32_t *indices, const voe_render_vertex *vertices,
		     voe_math_float3 eye, uint32_t a, uint32_t b, uint32_t c)
{
	voe_math_float3 corner = vertices[a].position;
	voe_math_float3 normal = voe_math_float3_cross(
		voe_math_float3_sub(vertices[b].position, corner),
		voe_math_float3_sub(vertices[c].position, corner));

	if (voe_math_float3_dot(normal, voe_math_float3_sub(eye, corner)) <
	    0.0f) {
		uint32_t swap = b;

		b = c;
		c = swap;
	}
	indices[0] = a;
	indices[1] = b;
	indices[2] = c;
}

void voe_3d_marker_lines(const voe_3d_marker_segment *segments, uint32_t count,
			 voe_math_float4x4 matrix, voe_render_view view,
			 voe_platform_size size, float pixels,
			 voe_base_arena *arena, voe_3d_outline_mesh *out)
{
	voe_render_vertex *vertices;
	uint32_t *indices;
	uint32_t quads = 0;

	VOE_BASE_ASSERT(arena != NULL, "marker lines with no arena");
	VOE_BASE_ASSERT(out != NULL, "marker lines into nothing");
	VOE_BASE_ASSERT(size.width != 0 && size.height != 0,
			"marker lines on a picture with no area");

	vertices = voe_base_arena_push(arena, sizeof *vertices * count * 4);
	indices = voe_base_arena_push(arena, sizeof *indices * count * 6);

	for (uint32_t i = 0; i < count; i++) {
		voe_math_float3 a = voe_math_float4x4_transform_point(
			matrix, segments[i].a);
		voe_math_float3 b = voe_math_float4x4_transform_point(
			matrix, segments[i].b);
		voe_math_float3 along = voe_math_float3_sub(b, a);
		voe_math_float3 side = voe_math_float3_cross(
			along, voe_math_float3_sub(a, view.eye));
		float near_width = half_width(view, size, pixels, a);
		float far_width = half_width(view, size, pixels, b);
		voe_math_float3 near_end;
		voe_math_float3 far_end;
		uint32_t v = quads * 4;

		if (voe_math_float3_length(side) < DEGENERATE)
			continue;
		along = voe_math_float3_normalize(along);
		side = voe_math_float3_normalize(side);

		near_end = voe_math_float3_sub(
			a, voe_math_float3_scale(along, near_width));
		far_end = voe_math_float3_add(
			b, voe_math_float3_scale(along, far_width));

		// Centred on the segment: a half width to each side of it.
		vertices[v + 0] = facing(
			voe_math_float3_sub(near_end, voe_math_float3_scale(
							      side, near_width)),
			view.eye);
		vertices[v + 1] = facing(
			voe_math_float3_sub(far_end, voe_math_float3_scale(
							     side, far_width)),
			view.eye);
		vertices[v + 2] = facing(
			voe_math_float3_add(near_end, voe_math_float3_scale(
							      side, near_width)),
			view.eye);
		vertices[v + 3] = facing(
			voe_math_float3_add(far_end, voe_math_float3_scale(
							     side, far_width)),
			view.eye);
		triangle(&indices[quads * 6 + 0], vertices, view.eye, v + 0,
			 v + 1, v + 2);
		triangle(&indices[quads * 6 + 3], vertices, view.eye, v + 1,
			 v + 3, v + 2);
		quads++;
	}

	*out = (voe_3d_outline_mesh){
		.vertices = vertices,
		.vertex_count = quads * 4,
		.indices = indices,
		.index_count = quads * 6,
	};
}

// The slab test: the ray's parameter range inside each pair of parallel faces,
// intersected. A direction with no component along an axis is inside that
// slab everywhere or nowhere, which is tested directly rather than divided by.
bool voe_3d_marker_box_hit(voe_math_float4x4 matrix, voe_math_float3 half,
			   voe_math_float3 direction, float *distance)
{
	voe_math_float4x4 inverse = voe_math_float4x4_inverse(matrix);
	voe_math_float3 o = voe_math_float4x4_transform_point(
		inverse, (voe_math_float3){ 0.0f, 0.0f, 0.0f });
	voe_math_float3 d = voe_math_float4x4_transform_dir(inverse, direction);
	const float origin[3] = { o.x, o.y, o.z };
	const float along[3] = { d.x, d.y, d.z };
	const float extent[3] = { half.x, half.y, half.z };
	float enter = -INFINITY;
	float leave = INFINITY;
	float at;

	for (int axis = 0; axis < 3; axis++) {
		float t0;
		float t1;

		if (along[axis] == 0.0f) {
			if (fabsf(origin[axis]) > extent[axis])
				return false;
			continue;
		}
		t0 = (-extent[axis] - origin[axis]) / along[axis];
		t1 = (extent[axis] - origin[axis]) / along[axis];
		if (t0 > t1) {
			float swap = t0;

			t0 = t1;
			t1 = swap;
		}
		enter = t0 > enter ? t0 : enter;
		leave = t1 < leave ? t1 : leave;
	}

	if (enter > leave || leave <= 0.0f)
		return false;
	at = enter > 0.0f ? enter : leave;
	if (distance != NULL)
		*distance = at;
	return true;
}
