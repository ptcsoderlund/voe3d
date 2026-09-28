// The silhouette walk and the quads it builds — see the header for what a
// silhouette edge is, why the quads stand about the eye and why the cap is a cap.
#include <3d/model_component.h>
#include <3d/outline.h>
#include <3d/shape_component.h>

#include <base/assert.h>

#include <math/float3.h>
#include <math/float4x4.h>

#include <scene/transform_component.h>

#include <stdbool.h>
#include <stddef.h>

// An edge seen so nearly end-on that the direction out of it cannot be worked
// out: the cross product below is in square metres, and a silhouette edge a
// millionth of that is a point on the picture with no quad in it. Below this it
// is left out rather than normalised, which asserts on nothing (math/float3.h).
#define DEGENERATE 1e-12f

// How far in front of the eye `p` is, in metres: the view matrix's third row is
// view-space Z, which runs backwards out of the screen.
static float depth_of(voe_render_view view, voe_math_float3 p)
{
	return -(view.view.m[2][0] * p.x + view.view.m[2][1] * p.y +
		 view.view.m[2][2] * p.z + view.view.m[2][3]);
}

// Half the line's width in metres at `p`: a metre at depth d covers
// projection.m[1][1] * height / (2 * d) pixels, so this is `pixels` pixels
// across at any distance — see the header.
static float half_width(voe_render_view view, voe_3d_outlined outlined,
			voe_math_float3 p)
{
	return depth_of(view, p) * outlined.pixels /
	       (view.projection.m[1][1] * (float)outlined.size.height);
}

// One corner of a quad, with its normal pointing at the eye.
static voe_render_vertex facing(voe_math_float3 position, voe_math_float3 eye)
{
	return (voe_render_vertex){
		.position = position,
		.normal = voe_math_float3_normalize(
			voe_math_float3_sub(eye, position)),
		.uv = { 0.0f, 0.0f },
	};
}

// One triangle of a quad, its last two corners swapped when it would face away
// from the eye and be culled.
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

// The CPU geometry `entity` is outlined from: its shape's kind, else the
// loaded entry of the model it wears; NULL when it has neither.
static const voe_3d_shape_geometry *
outlined_geometry(const voe_ecs_world *world, voe_3d_outlined outlined)
{
	const voe_3d_shape *shape = voe_3d_shape_get(world, outlined.entity);
	const voe_3d_model *model;
	const voe_3d_model_entry *entry;

	VOE_BASE_ASSERT(world != NULL, "a geometry of no world");
	if (shape != NULL)
		return outlined.geometries == NULL ?
			       NULL :
			       voe_3d_shape_geometry_of(outlined.geometries,
							shape->kind);
	if (outlined.models == NULL)
		return NULL;
	model = voe_3d_model_get(world, outlined.entity);
	if (model == NULL)
		return NULL;
	entry = voe_3d_models_find(outlined.models, model->path);
	return entry != NULL && entry->loaded ? &entry->shape : NULL;
}

bool voe_3d_outline_quads(const voe_ecs_world *world, voe_3d_outlined outlined,
			  voe_render_view view, voe_math_double3 eye,
			  voe_base_arena *arena, voe_3d_outline_mesh *out)
{
	const voe_3d_shape_geometry *geometry;
	struct voe_base_arena_mark mark;
	voe_math_float4x4 matrix;
	voe_math_float3 eye_local;
	voe_render_vertex *vertices;
	uint32_t *indices;
	uint32_t quads = 0;

	VOE_BASE_ASSERT(world != NULL, "outlining an entity of no world");
	VOE_BASE_ASSERT(arena != NULL, "outlining with no arena");
	VOE_BASE_ASSERT(out != NULL, "outlining into nothing");

	if (voe_scene_transform_get(world, outlined.entity) == NULL)
		return false;
	geometry = outlined_geometry(world, outlined);
	if (geometry == NULL)
		return false;

	// About the eye, as `view` is (ADR-0250): the quads are eye-relative,
	// and stand about the entity's world place (0281).
	matrix = voe_scene_transform_matrix(
		voe_scene_transform_world(world, outlined.entity), eye);
	// Scaled away to nothing: drawn as nothing, so there is no outline of
	// it, and its matrix cannot be inverted (math/float4x4.h asserts on a
	// singular one) — 3d/pick.c skips such an entity for the same reason.
	if (voe_math_float4x4_determinant(matrix) == 0.0f)
		return false;

	// The eye in the shape's own space, which is where the edges and their
	// normals are.
	eye_local = voe_math_float4x4_transform_point(
		voe_math_float4x4_inverse(matrix), view.eye);

	mark = voe_base_arena_mark(arena);
	vertices = voe_base_arena_push(arena,
				       sizeof *vertices * VOE_3D_OUTLINE_VERTICES);
	indices = voe_base_arena_push(arena,
				      sizeof *indices * VOE_3D_OUTLINE_INDICES);

	for (uint32_t i = 0;
	     i < geometry->edge_count && quads < VOE_3D_OUTLINE_EDGES; i++) {
		const voe_3d_shape_edge *edge = &geometry->edges[i];
		voe_math_float3 to_eye =
			voe_math_float3_sub(eye_local, edge->a);
		bool left_front = voe_math_float3_dot(edge->left, to_eye) > 0.0f;
		bool right_front =
			voe_math_float3_dot(edge->right, to_eye) > 0.0f;
		voe_math_float3 a;
		voe_math_float3 b;
		voe_math_float3 along;
		voe_math_float3 outward;
		voe_math_float3 front;
		voe_math_float3 near_end;
		voe_math_float3 far_end;
		float near_width;
		float far_width;
		uint32_t v = quads * 4;

		// The two triangles agree about which side of them the eye is
		// on, so the surface does not turn away here.
		if (left_front == right_front)
			continue;

		a = voe_math_float4x4_transform_point(matrix, edge->a);
		b = voe_math_float4x4_transform_point(matrix, edge->b);
		along = voe_math_float3_sub(b, a);
		outward = voe_math_float3_cross(
			along, voe_math_float3_sub(a, view.eye));
		if (voe_math_float3_length(outward) < DEGENERATE)
			continue;
		along = voe_math_float3_normalize(along);
		outward = voe_math_float3_normalize(outward);

		// Out of the shape and not into it: the normal of whichever of
		// the two triangles the eye is in front of, about the eye.
		front = voe_math_float4x4_transform_dir(
			matrix, left_front ? edge->left : edge->right);
		if (voe_math_float3_dot(outward, front) < 0.0f)
			outward = voe_math_float3_neg(outward);

		near_width = half_width(view, outlined, a);
		far_width = half_width(view, outlined, b);
		// Half a width past each end, so a corner between two quads has
		// no notch in it.
		near_end = voe_math_float3_sub(
			a, voe_math_float3_scale(along, near_width));
		far_end = voe_math_float3_add(
			b, voe_math_float3_scale(along, far_width));

		vertices[v + 0] = facing(near_end, view.eye);
		vertices[v + 1] = facing(far_end, view.eye);
		vertices[v + 2] = facing(
			voe_math_float3_add(near_end,
					    voe_math_float3_scale(
						    outward, 2.0f * near_width)),
			view.eye);
		vertices[v + 3] = facing(
			voe_math_float3_add(far_end,
					    voe_math_float3_scale(
						    outward, 2.0f * far_width)),
			view.eye);
		triangle(&indices[quads * 6 + 0], vertices, view.eye, v + 0,
			 v + 1, v + 2);
		triangle(&indices[quads * 6 + 3], vertices, view.eye, v + 1,
			 v + 3, v + 2);
		quads++;
	}

	if (quads == 0) {
		voe_base_arena_rewind(arena, mark);
		return false;
	}

	*out = (voe_3d_outline_mesh){
		.vertices = vertices,
		.vertex_count = quads * 4,
		.indices = indices,
		.index_count = quads * 6,
	};
	return true;
}
