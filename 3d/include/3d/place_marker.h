// A place with no mesh drawn in an editor's view as a marker (0365 points 1-2):
// a wire diamond about its position, as line quads about the eye, the ray that
// picks it, and the one answer to which entities wear it. It builds two arrays
// in an arena and uploads nothing; the draw system is what hands them to the card.
//
//     voe_3d_outline_mesh mesh;
//
//     if (voe_3d_place_marker_wanted(world, entity) &&
//         voe_3d_place_marker_quads(position, view, eye, size, 2.0f, arena,
//                                   &mesh))
//             ... // mesh.vertices, mesh.indices: this frame's geometry
//     if (voe_3d_place_marker_hit(position, ray, &distance))
//             ... // the place is under the pointer
//
// A WIRE DIAMOND, BECAUSE A PLACE IS A POINT WITH NO DIRECTION. An octahedron's
// twelve edges, its six tips 0.25 m along the world's ±X, ±Y and ±Z, read as
// "a point here" from any side and differ from the lamp's ball
// (3d/point_light_marker.h) and the sun's arrow (3d/sun_marker.h), which would
// say a light or a way it shines. They are 3d/outline.h's quads, centred on
// each edge: a fixed pixel width from each end's own depth, wound to face the
// eye, in the caller's arena (rule 11). An edge seen exactly end-on is left out.
//
// WORLD AXES AND POSITION ONLY, as the lamp's: an entity turned or scaled to
// nothing is still seen and picked where it stands.
//
// A CUBE IS HIT AND NOT THE LINES, as every marker's is: thin lines are a few
// pixels a person would have to land on exactly, and a world-axis cube of half
// extent 0.25 m about the position is the diamond's reach.
//
// THE DRAW AND THE PICK BOTH ASK voe_3d_place_marker_wanted, so what is drawn
// is exactly what is clicked: one predicate, never two lists that drift.
#pragma once

#include <3d/outline.h>
#include <3d/pick.h>

#include <base/arena.h>

#include <ecs/world.h>

#include <math/double3.h>

#include <render/device.h>

#include <stdbool.h>

// The diamond's twelve edges, and the vertices and indices they come to, four
// and six an edge.
#define VOE_3D_PLACE_MARKER_EDGES 12
#define VOE_3D_PLACE_MARKER_VERTICES (VOE_3D_PLACE_MARKER_EDGES * 4)
#define VOE_3D_PLACE_MARKER_INDICES (VOE_3D_PLACE_MARKER_EDGES * 6)

// Builds the marker of a place at world `position` as seen through `view` from
// `eye`, in metres about `eye` in float (ADR-0250), `pixels` wide on a picture
// `size` pixels big, into `arena`, and fills `out`.
//
// False, with `out` untouched, when `size` has no area.
[[nodiscard]] bool voe_3d_place_marker_quads(voe_math_double3 position,
					     voe_render_view view,
					     voe_math_double3 eye,
					     voe_platform_size size,
					     float pixels, voe_base_arena *arena,
					     voe_3d_outline_mesh *out);

// Whether `ray` meets the cube of a place at world `position`, and how far along
// it: `distance`, which may be NULL, is where it enters the cube, or where it
// leaves it when the ray starts inside, in metres along the world ray. False
// for no hit in front of the ray's origin.
[[nodiscard]] bool voe_3d_place_marker_hit(voe_math_double3 position,
					   voe_3d_ray ray, float *distance);

// Whether `entity` wears the marker: alive, with a transform, and with no row
// in the shape, model, water, mesh, panel, camera, light or point light table.
// A table the world never registered counts as no row.
[[nodiscard]] bool voe_3d_place_marker_wanted(const voe_ecs_world *world,
					      voe_ecs_entity entity);
