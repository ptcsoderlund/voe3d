// A collider drawn in an editor's view as lines (0253): a box's twelve edges,
// a sphere's three great circles, a capsule's end circles, upright outlines and
// cap arcs, each as a line quad about the eye. It builds two arrays in an arena
// and uploads nothing; the draw system is what hands them to the card.
//
//     voe_physics_shape shape;
//     voe_3d_outline_mesh mesh;
//
//     if (voe_physics_shape_of(world, entity, &shape) &&
//         voe_3d_collider_marker_quads(shape, view, eye, size, 2.0f, arena,
//                                      &mesh))
//             ... // mesh.vertices, mesh.indices: this frame's geometry
//
// THE SHAPE IS physics's WORLD SHAPE (physics/shape.h): centre, rotation and
// half sizes with the transform's scale already applied, so a scaled entity's
// lines scale with it and are exactly what a query tests against.
//
// LINES AND NOT A SOLID, because a collider is drawn over the thing it fits:
// a solid would hide that thing, and a see-through one would need the blended
// group's sort. Lines show both the collider and what is inside it.
//
// THE LINES ARE 3d/outline.h's QUADS, centred on each segment as the camera
// marker's are (3d/camera_marker.h): a fixed pixel width from each end's own
// depth, extended half a width past both ends, wound to face the eye, in the
// caller's arena (rule 11). A circle is 24 segments. A segment seen exactly
// end-on, or of no length, is left out.
#pragma once

#include <3d/outline.h>

#include <base/arena.h>

#include <math/double3.h>

#include <physics/shape.h>

#include <render/device.h>

// The most segments any kind needs — a capsule's: two end circles, four
// upright lines and four half arcs of its caps — and the vertices and indices
// they come to, four and six a segment.
#define VOE_3D_COLLIDER_MARKER_CIRCLE 24
#define VOE_3D_COLLIDER_MARKER_EDGES                                   \
	(2 * VOE_3D_COLLIDER_MARKER_CIRCLE + 4 +                       \
	 4 * (VOE_3D_COLLIDER_MARKER_CIRCLE / 2))
#define VOE_3D_COLLIDER_MARKER_VERTICES (VOE_3D_COLLIDER_MARKER_EDGES * 4)
#define VOE_3D_COLLIDER_MARKER_INDICES (VOE_3D_COLLIDER_MARKER_EDGES * 6)

// Builds the lines of `shape` as seen through `view` from `eye`, in metres
// about `eye` (ADR-0250), `pixels` wide on a picture `size` pixels big, into
// `arena`, and fills `out`.
//
// False, with `out` untouched, when `size` has no area or the kind is not one
// physics defines.
[[nodiscard]] bool voe_3d_collider_marker_quads(voe_physics_shape shape,
						voe_render_view view,
						voe_math_double3 eye,
						voe_platform_size size,
						float pixels,
						voe_base_arena *arena,
						voe_3d_outline_mesh *out);
