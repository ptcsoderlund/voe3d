// A sculpting brush drawn in an editor's view (0379 point 3): two rings lying
// on a landscape's ground, as line quads about the eye. It builds two arrays
// in an arena and uploads nothing; the draw system hands them to the card.
//
//     voe_3d_outline_mesh mesh;
//
//     if (voe_3d_brush_marker_quads(land, pose, x, z, radius, inner, view,
//                                   eye, size, 2.0f, arena, &mesh))
//             ... // mesh.vertices, mesh.indices: this frame's geometry
//
// LINES ON THE GROUND, BECAUSE THE BRUSH IS A PATCH OF GROUND. What a stamp
// changes is the heights inside a circle, so the circle follows those heights:
// each ring point sits at voe_3d_landscape_height plus 5 cm, so a ring over a
// hill bends with it and is not hidden inside it. Lines and not a disc, so the
// ground being shaped stays visible under the mark.
//
// TWO RINGS, BECAUSE A BRUSH HAS TWO RADII. Inside `inner`, the radius times
// one minus softness, the stamp is at full weight; between it and `radius` it
// fades (3d/landscape.h). Showing both tells a person how soft the edge is
// without a stamp. An `inner` of nought builds a ring of no length, whose
// segments are left out, so the mesh may hold fewer quads than the defines.
//
// THE GRID'S OWN SPACE: (x, z) and the radii are about the thing's origin as
// the heights are, and `pose` — the row's world place, scale included, as the
// landscape is drawn — takes the rings to the world about `eye` (ADR-0250).
// The quads are 3d/outline.h's, through marker_lines.h, `pixels` wide from
// each end's own depth, in the caller's arena (rule 11).
#pragma once

#include <3d/outline.h>

#include <assets/landscape.h>
#include <base/arena.h>

#include <math/double3.h>

#include <render/device.h>

#include <scene/transform_component.h>

// Segments per ring, the rings' edges, and the vertices and indices they come
// to, four and six an edge.
#define VOE_3D_BRUSH_MARKER_SEGMENTS 48
#define VOE_3D_BRUSH_MARKER_EDGES (2 * VOE_3D_BRUSH_MARKER_SEGMENTS)
#define VOE_3D_BRUSH_MARKER_VERTICES (VOE_3D_BRUSH_MARKER_EDGES * 4)
#define VOE_3D_BRUSH_MARKER_INDICES (VOE_3D_BRUSH_MARKER_EDGES * 6)

// Builds both rings of a brush at (`x`, `z`) on `landscape`, placed by `pose`,
// as seen through `view` from `eye`, `pixels` wide on a picture `size` pixels
// big, into `arena`, and fills `out`: the `radius` ring's edges first, then
// the `inner` one's. False, with `out` untouched, when `size` has no area.
[[nodiscard]] bool
voe_3d_brush_marker_quads(const voe_assets_landscape *landscape,
			  voe_scene_transform pose, float x, float z,
			  float radius, float inner, voe_render_view view,
			  voe_math_double3 eye, voe_platform_size size,
			  float pixels, voe_base_arena *arena,
			  voe_3d_outline_mesh *out);
