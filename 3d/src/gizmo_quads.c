// The gizmos' shared quads: a build's two arrays in an arena, each corner's
// normal towards the eye, each triangle's winding swapped when it would face
// away, and a quad as two of those triangles. See gizmo_quads.h.
#include "gizmo_quads.h"

#include <base/arena.h>
#include <base/assert.h>

#include <math/float3.h>

#include <stddef.h>

struct voe_3d_gizmo_build voe_3d_gizmo_build_in(voe_base_arena *arena,
						uint32_t vertices,
						uint32_t indices)
{
	struct voe_3d_gizmo_build mesh = {
		.vertex_capacity = vertices,
		.index_capacity = indices,
	};

	VOE_BASE_ASSERT(arena != NULL, "a mesh built with no arena");
	VOE_BASE_ASSERT(vertices > 0 && indices > 0, "a mesh with no room");
	mesh.vertices = voe_base_arena_push(arena,
					    sizeof *mesh.vertices * vertices);
	mesh.indices = voe_base_arena_push(arena,
					   sizeof *mesh.indices * indices);
	return mesh;
}

voe_render_vertex voe_3d_gizmo_facing(voe_math_float3 position,
				      voe_math_float3 eye)
{
	return (voe_render_vertex){
		.position = position,
		.normal = voe_math_float3_normalize(
			voe_math_float3_sub(eye, position)),
		.uv = { 0.0f, 0.0f },
	};
}

// The pipeline culls back faces, so the last two corners are swapped when the
// triangle would face away.
void voe_3d_gizmo_wind(struct voe_3d_gizmo_build *mesh, uint32_t a, uint32_t b,
		       uint32_t c, voe_math_float3 eye)
{
	voe_math_float3 corner = mesh->vertices[a].position;
	voe_math_float3 normal = voe_math_float3_cross(
		voe_math_float3_sub(mesh->vertices[b].position, corner),
		voe_math_float3_sub(mesh->vertices[c].position, corner));

	VOE_BASE_ASSERT(mesh->index_count + 3 <= mesh->index_capacity,
			"more indices than the mesh has room for");
	VOE_BASE_ASSERT(a < mesh->vertex_count && b < mesh->vertex_count &&
				c < mesh->vertex_count,
			"a triangle of corners nobody pushed");
	if (voe_math_float3_dot(normal, voe_math_float3_sub(eye, corner)) <
	    0.0f) {
		uint32_t swap = b;

		b = c;
		c = swap;
	}
	mesh->indices[mesh->index_count++] = a;
	mesh->indices[mesh->index_count++] = b;
	mesh->indices[mesh->index_count++] = c;
}

void voe_3d_gizmo_add_quad(struct voe_3d_gizmo_build *mesh,
			   voe_math_float3 from, voe_math_float3 to,
			   voe_math_float3 across, float half_width,
			   voe_math_float3 eye)
{
	voe_math_float3 offset = voe_math_float3_scale(across, half_width);
	uint32_t v = mesh->vertex_count;

	VOE_BASE_ASSERT(mesh->vertex_count + 4 <= mesh->vertex_capacity,
			"more vertices than the mesh has room for");
	VOE_BASE_ASSERT(half_width > 0.0f, "a quad of no width");
	mesh->vertices[v + 0] =
		voe_3d_gizmo_facing(voe_math_float3_sub(from, offset), eye);
	mesh->vertices[v + 1] =
		voe_3d_gizmo_facing(voe_math_float3_add(from, offset), eye);
	mesh->vertices[v + 2] =
		voe_3d_gizmo_facing(voe_math_float3_sub(to, offset), eye);
	mesh->vertices[v + 3] =
		voe_3d_gizmo_facing(voe_math_float3_add(to, offset), eye);
	mesh->vertex_count += 4;
	voe_3d_gizmo_wind(mesh, v + 0, v + 1, v + 2, eye);
	voe_3d_gizmo_wind(mesh, v + 1, v + 3, v + 2, eye);
}

voe_3d_gizmo_mesh voe_3d_gizmo_mesh_of(struct voe_3d_gizmo_build mesh)
{
	VOE_BASE_ASSERT(mesh.vertex_count <= mesh.vertex_capacity,
			"a mesh of more vertices than it has room for");
	VOE_BASE_ASSERT(mesh.index_count <= mesh.index_capacity,
			"a mesh of more indices than it has room for");
	return (voe_3d_gizmo_mesh){
		.vertices = mesh.vertices,
		.vertex_count = mesh.vertex_count,
		.indices = mesh.indices,
		.index_count = mesh.index_count,
	};
}
