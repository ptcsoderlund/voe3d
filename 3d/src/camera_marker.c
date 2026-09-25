// The camera marker's edges and quads, and the ray against its box — see the
// header for the geometry, why the quads are outline.h's and why only the box
// is hit.
#include <3d/camera_marker.h>

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

// outline.c's: an edge whose cross product with the eye is below this is seen
// end-on and has no quad.
#define DEGENERATE 1e-12f

typedef struct {
	voe_math_float3 a;
	voe_math_float3 b;
} segment;

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
	voe_render_vertex *vertices;
	uint32_t *indices;
	uint32_t quads = 0;

	VOE_BASE_ASSERT(arena != NULL, "a camera marker with no arena");
	VOE_BASE_ASSERT(out != NULL, "a camera marker into nothing");

	if (voe_math_float4x4_determinant(matrix) == 0.0f ||
	    size.width == 0 || size.height == 0)
		return false;

	marker_edges(lens, edges);
	vertices = voe_base_arena_push(
		arena, sizeof *vertices * VOE_3D_CAMERA_MARKER_VERTICES);
	indices = voe_base_arena_push(
		arena, sizeof *indices * VOE_3D_CAMERA_MARKER_INDICES);

	for (uint32_t i = 0; i < VOE_3D_CAMERA_MARKER_EDGES; i++) {
		voe_math_float3 a =
			voe_math_float4x4_transform_point(matrix, edges[i].a);
		voe_math_float3 b =
			voe_math_float4x4_transform_point(matrix, edges[i].b);
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

		// Centred on the edge: a half width to each side of it.
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
	return true;
}

// The slab test: the ray's parameter range inside each pair of parallel faces,
// intersected. A direction with no component along an axis is inside that
// slab everywhere or nowhere, which is tested directly rather than divided by.
bool voe_3d_camera_marker_hit(voe_scene_transform pose, voe_3d_ray ray,
			      float *distance)
{
	// About the ray's own origin, so the ray starts at nought in float and
	// only the small difference is narrowed (ADR-0250).
	voe_math_float4x4 matrix = voe_scene_transform_matrix(pose, ray.origin);
	voe_math_float4x4 inverse;
	float origin[3];
	float direction[3];
	const float half[3] = { HALF_X, HALF_Y, HALF_Z };
	float enter = -INFINITY;
	float leave = INFINITY;
	float at;

	if (voe_math_float4x4_determinant(matrix) == 0.0f)
		return false;
	inverse = voe_math_float4x4_inverse(matrix);

	{
		voe_math_float3 o =
			voe_math_float4x4_transform_point(
				inverse, (voe_math_float3){ 0.0f, 0.0f, 0.0f });
		voe_math_float3 d =
			voe_math_float4x4_transform_dir(inverse, ray.direction);

		origin[0] = o.x;
		origin[1] = o.y;
		origin[2] = o.z;
		direction[0] = d.x;
		direction[1] = d.y;
		direction[2] = d.z;
	}

	for (int axis = 0; axis < 3; axis++) {
		float t0;
		float t1;

		if (direction[axis] == 0.0f) {
			if (fabsf(origin[axis]) > half[axis])
				return false;
			continue;
		}
		t0 = (-half[axis] - origin[axis]) / direction[axis];
		t1 = (half[axis] - origin[axis]) / direction[axis];
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
