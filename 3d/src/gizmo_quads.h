// The camera-facing quads the move gizmo and the rotate rings are both drawn
// as: one mesh under construction in the caller's arena, a corner whose normal
// points at the eye, a triangle wound to face it and a quad widened along a
// given direction. Internal to 3d: gizmo.c and gizmo_rings.c build with these.
//
//     struct voe_3d_gizmo_build mesh =
//             voe_3d_gizmo_build_in(arena, vertices, indices);
//     voe_3d_gizmo_add_quad(&mesh, from, to, across, half_width, eye);
//     *out = voe_3d_gizmo_mesh_of(mesh);
//
// Everything is about the eye (ADR-0250), so `eye` is whatever point the
// positions are measured from. A build holds its own capacity and asserts it is
// never passed: the counts are the caller's header constants, and a push past
// them is a bug and not an error to report.
#pragma once

#include <3d/gizmo.h>

#include <base/arena.h>

#include <math/float3.h>

#include <render/device.h>

#include <stdint.h>

// One mesh under construction: its two arrays, how much of them is used and
// how much there is.
struct voe_3d_gizmo_build {
	voe_render_vertex *vertices;
	uint32_t *indices;
	uint32_t vertex_count;
	uint32_t index_count;
	uint32_t vertex_capacity;
	uint32_t index_capacity;
};

// An empty build with room for `vertices` and `indices`, pushed into `arena`.
struct voe_3d_gizmo_build voe_3d_gizmo_build_in(voe_base_arena *arena,
						uint32_t vertices,
						uint32_t indices);

// One corner, with its normal the direction from it to `eye`.
voe_render_vertex voe_3d_gizmo_facing(voe_math_float3 position,
				      voe_math_float3 eye);

// Three corners already pushed, wound to face `eye`.
void voe_3d_gizmo_wind(struct voe_3d_gizmo_build *mesh, uint32_t a, uint32_t b,
		       uint32_t c, voe_math_float3 eye);

// The quad from `from` to `to`, widened by `half_width` to each side along
// `across`, a unit vector.
void voe_3d_gizmo_add_quad(struct voe_3d_gizmo_build *mesh,
			   voe_math_float3 from, voe_math_float3 to,
			   voe_math_float3 across, float half_width,
			   voe_math_float3 eye);

// What one build has come to, as the mesh the caller was promised.
voe_3d_gizmo_mesh voe_3d_gizmo_mesh_of(struct voe_3d_gizmo_build mesh);
