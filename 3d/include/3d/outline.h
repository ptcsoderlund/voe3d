// One entity's silhouette, seen from one camera, as quads in metres about the
// eye (ADR-0250): the geometry a selection outline is drawn from (ADR-0203). It builds
// two arrays in an arena and uploads nothing; 3d/draw_system.h is what hands
// them to the card.
//
//     voe_3d_outline_mesh mesh;
//     voe_3d_outlined outlined = { .entity = selected,
//                                  .geometries = &geometries,
//                                  .models = models,
//                                  .material = shapes.outline,
//                                  .colour = theme_colour,
//                                  .pixels = 2.0f,
//                                  .size = view_size };
//
//     if (voe_3d_outline_quads(world, outlined, view, eye, arena, &mesh))
//             ... // mesh.vertices, mesh.indices: this frame's geometry
//
// WHAT A SILHOUETTE EDGE IS. An edge of the surface whose two triangles
// disagree about which side of them the eye is on: one faces the eye, the other
// faces away. The set of them is exactly the outline the drawn triangles have,
// because the picture's outline is where the surface turns away from the eye,
// and that turn happens along an edge and nowhere else. Nothing here looks at a
// pixel: it is a walk over the edges 3d/shape_geometry.h already welded, in the
// shape's own space, with the eye carried into it by the inverse of the world
// matrix.
//
// AN EDGE WHOSE TWO TRIANGLES SHARE A NORMAL IS NOT ONE. Both of them agree
// about the eye, always, so it never shows: that is a cube's six face diagonals,
// which are a fold of nothing and must not be drawn across its faces. The same
// answer is given to the boundary edge of an open surface, which carries one
// triangle's normal in both (3d/shape_geometry.h) and would be on the outline —
// all three built-in shapes are closed, and a model's open edge goes undrawn
// until that distinction is carried in the edge.
//
// THE QUADS STAND ABOUT THE EYE AND NOT ON THE SCREEN. They are drawn through
// the same camera and the same two matrices as everything else in the view, so
// there is no second space here, no orthographic projection and no screen-space
// pass — 3d/draw_system.h says the same about the overlay layer, which is a
// depth clear and not a different way of drawing. A quad's corner goes through
// the matrix about the eye once, here, and is eye-relative from then on.
//
// THEY ARE WOUND TO FACE THE EYE AND CARRY NORMALS POINTING AT IT. The draw
// pipeline culls back faces (render/device.h), so a quad wound the other way
// would be a hole in the outline that appears and disappears as the object
// turns; each triangle is checked against the eye and its last two corners
// swapped when it is wrong. The normals are the eye's direction even though the
// record the quads wear is unlit and nothing reads them, because a zeroed normal
// means something of its own to the shader (render/device.h) and a vertex is
// cheaper to fill correctly than to explain.
//
// EVERYTHING IT HANDS BACK IS THE CALLER'S ARENA'S (rule 11) and dies with it:
// a frame arena rewound at the top of the next frame, which is exactly how long
// this frame's geometry is wanted.
#pragma once

#include <3d/material_component.h>
#include <3d/models.h>
#include <3d/shape_geometry.h>

#include <base/arena.h>

#include <ecs/world.h>

#include <math/double3.h>

#include <render/device.h>

// VOE_3D_OUTLINE_EDGES IS A CAP AND NOT AN ASSERT. A capsule is about fifteen
// hundred edges and a model read from a file has no measured number at all; a
// shape nobody has measured must not be able to fill one frame's transient pool
// and have the frame refused. Past the cap the outline is missing a few edges,
// which is a line with a gap in it, and everything else in the frame is drawn.
// It is 4096 and not the shapes' 512 because a model's silhouette is walked
// over thousands of triangles, and its turning edges run to a few thousand.
//
// The most edges one outline is built from, and the vertices and indices that
// many edges come to. A program sizes voe_render_capacities' three transient
// numbers from these: four vertices and six indices per edge, one transient
// range per view that outlines something.
#define VOE_3D_OUTLINE_EDGES 4096
#define VOE_3D_OUTLINE_VERTICES (VOE_3D_OUTLINE_EDGES * 4)
#define VOE_3D_OUTLINE_INDICES (VOE_3D_OUTLINE_EDGES * 6)

// What a pass outlines and how. A zeroed entity outlines nothing, which is what
// no selection looks like (ecs/world.h), and a NULL geometry store the same.
typedef struct {
	voe_ecs_entity entity;
	// The CPU-side geometry the silhouette is walked over, or NULL.
	const voe_3d_shape_geometries *geometries;
	// The model store an entity with no shape is outlined from, or NULL
	// for none (ADR-0277 point 3); a world handed one registers models,
	// as voe_3d_frame.models asks.
	const voe_3d_models *models;
	// The unlit record the quads wear — voe_3d_shapes' outline material.
	// Read by whoever draws them, not by the build below.
	voe_3d_material material;
	// Linear, and the whole of what the outline looks like, because the
	// record is unlit and white (ADR-0203).
	voe_math_float3 colour;
	// How wide the line is on the picture, in pixels, at any distance.
	float pixels;
	// The size of that picture, in pixels. The height is what the width is
	// worked out against, because the projection's vertical field of view
	// is.
	voe_platform_size size;
} voe_3d_outlined;

// The silhouette as geometry, all of it in the arena the build was handed.
typedef struct {
	const voe_render_vertex *vertices;
	uint32_t vertex_count;
	const uint32_t *indices;
	uint32_t index_count;
} voe_3d_outline_mesh;

// THE WIDTH IS WORKED OUT PER VERTEX FROM THAT VERTEX'S OWN DEPTH, which is
// what keeps the line the same thickness far away as near. A metre at depth `d`
// covers `projection.m[1][1] * height / (2 * d)` pixels, so the half width that
// covers `pixels` pixels is `d * pixels / (projection.m[1][1] * height)` — the
// projection's second diagonal being one over the tangent of half the vertical
// field of view (3d/projection.h). Per vertex and not per edge, because an edge
// running away from the eye is nearer at one end than at the other and a single
// width would taper wrongly.
//
// THE QUADS ARE EXTENDED HALF A WIDTH PAST EACH END. Two quads meeting at a
// corner of the silhouette are two rectangles at an angle, and rectangles that
// stopped exactly at the shared corner would leave a notch in it the width of
// the line. Half a width at each end fills that corner for any angle a
// silhouette makes.
//
// Builds `outlined`'s silhouette as seen through `view` from `eye`, in metres
// about `eye`, into `arena` and fills `out`.
//
// False, with nothing written to `out`, when there is nothing to outline: a
// zeroed or dead entity, no store, an entity with no transform, one with no
// shape and no model with a loaded entry (or no model store), a kind this
// build does not know, one scaled away to nothing (3d/pick.h skips
// the same entity for the same reason), or a silhouette with no edges at all —
// which is a caller's ordinary case and not a failure, so the frame simply
// draws no outline.
[[nodiscard]] bool voe_3d_outline_quads(const voe_ecs_world *world,
					voe_3d_outlined outlined,
					voe_render_view view,
					voe_math_double3 eye,
					voe_base_arena *arena,
					voe_3d_outline_mesh *out);
