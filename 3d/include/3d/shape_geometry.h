// The three built-in shapes as the CPU sees them: the triangles each one is
// built from, and the edges of its surface with the two triangle normals that
// meet along each. Built once at startup into an arena, read-only afterwards,
// and keyed by the same kind number a shape component holds.
//
//     voe_3d_shape_geometries geometries;
//
//     voe_3d_shape_geometries_create(arena, &geometries);
//     const voe_3d_shape_geometry *g =
//             voe_3d_shape_geometry_of(&geometries, row->kind);
//
// NOTHING HERE TALKS TO A DEVICE. voe_3d_shapes_upload (3d/shape_system.h) hands
// the same triangles to the card; this is the copy that stays where the CPU can
// walk it, and the two are built from the same three builders so neither can
// drift from the other.
//
// WHY THE CPU KEEPS A COPY AT ALL. A click is answered by a ray cast against the
// shape's own triangles (ADR-0202), and a ray cannot be cast at a buffer that
// lives on the card: reading one back waits for the card to go idle and may not
// be done inside a frame. A selection outline is the silhouette walked over the
// surface's edges (ADR-0203), which is a walk over geometry and not over pixels.
// Both want the same geometry in the same place, so there is one store for both.
//
// IT IS BUILT AND NOT WRITTEN OUT. ../../src/cube.h, ../../src/capsule.h and
// ../../src/cylinder.h already produce every vertex and index of these shapes
// for the upload; a second hand-written copy here would be a second truth, and
// the day one of them gained a segment the ray would be cast at the old shape.
// The cube's arrays are constants and are pointed straight at; the capsule's and
// the cylinder's are built into the caller's arena.
//
// IT IS KEYED BY KIND, because a kind — not a mesh and not a material — is what
// a saved scene holds (ADR-0191, 3d/shape_component.h). Three kinds is the whole
// of what an editor's project can draw, and about 250 KB of arena covers all
// three. Triangles read from a model file are a later card and will be kept the
// same way, beside the mesh they were read for.
//
// THE ARENA IS THE CALLER'S AND MUST OUTLIVE THE ANSWER (rule 11). Everything
// handed back points into it; rewinding or destroying it invalidates every
// pointer in the store at once, and nothing here is freed one allocation at a
// time.
#pragma once

#include <base/arena.h>
#include <math/float3.h>
#include <render/device.h>

#include <stdint.h>

// Two positions are the same vertex when every component of the difference is
// smaller than this: a tenth of a millimetre, on shapes about a metre across.
// Small enough that two corners of a cube a metre apart are never confused, and
// large enough to cover the arithmetic that placed a capsule's seam twice.
#define VOE_3D_SHAPE_GEOMETRY_WELD 1e-4f

// One edge of a shape's surface, in the shape's own space: where it runs, and
// the outward normal of each of the two triangles that meet along it.
//
// THE WELD IS BY POSITION AND NOT BY INDEX. A cube's corner is three vertices
// and a capsule's seam two, because a vertex carries one normal and one texture
// coordinate and the same point needs several of each (../../src/cube.h says the
// same thing); edges keyed by index would split every fold of the surface into
// two halves that never meet, and the silhouette walked over them would be the
// whole surface rather than its outline.
//
// A CLOSED SURFACE LEAVES EVERY EDGE WITH TWO TRIANGLES, which is what all three
// built-in shapes are, so `left` and `right` are two real normals for every edge
// of them. An edge only one triangle reaches — the boundary of an open surface —
// carries that triangle's normal in both, which is not a fault: it is an edge
// that is always on the silhouette, because it faces the eye the same way from
// both sides and never straddles it.
typedef struct {
	voe_math_float3 a;
	voe_math_float3 b;
	voe_math_float3 left;
	voe_math_float3 right;
} voe_3d_shape_edge;

// One kind's geometry. Every pointer is into the arena the store was created
// with, or at a constant in this build, and none of it changes afterwards.
typedef struct {
	const voe_render_vertex *vertices;
	uint32_t vertex_count;
	const uint32_t *indices;
	uint32_t index_count;
	const voe_3d_shape_edge *edges;
	uint32_t edge_count;
} voe_3d_shape_geometry;

// Every kind's geometry, indexed by the kind's own number, exactly as
// voe_3d_shape_kind_names is (3d/shape_component.h): entry nought is zeroed,
// because nought is no kind.
typedef struct {
	voe_3d_shape_geometry kinds[4];
} voe_3d_shape_geometries;

// Builds all three kinds into `arena` and fills `out`. A startup operation with
// no device in it: it draws nothing, uploads nothing and can run on a machine
// with no graphics card.
//
// What it hands back is read-only and lives in `arena` until that arena is
// rewound past this call or destroyed (rule 11). Its own working memory — the
// welded vertex numbers and the table the edges are found in — is taken from the
// same arena and given back before it returns.
void voe_3d_shape_geometries_create(voe_base_arena *arena,
				    voe_3d_shape_geometries *out);

// The entry for `kind`, or NULL for nought and for a kind this build does not
// know — the same answer voe_3d_shape_kind_names gives such a kind, so a caller
// that already draws nothing for an unknown kind picks and outlines nothing for
// it too.
const voe_3d_shape_geometry *
voe_3d_shape_geometry_of(const voe_3d_shape_geometries *geometries,
			 uint32_t kind);
