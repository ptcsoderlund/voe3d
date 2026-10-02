// A point light drawn in an editor's view as a marker (0320 point 7): three wire
// circles about its position, as line quads about the eye, and the ray that
// picks it. It builds two arrays in an arena and uploads nothing; the draw
// system is what hands them to the card.
//
//     voe_3d_outline_mesh mesh;
//
//     if (voe_3d_point_light_marker_quads(position, view, eye, size, 2.0f,
//                                         arena, &mesh))
//             ... // mesh.vertices, mesh.indices: this frame's geometry
//     if (voe_3d_point_light_marker_hit(position, ray, &distance))
//             ... // the lamp is under the pointer
//
// A WIRE BALL, BECAUSE A LAMP HAS A PLACE AND NO DIRECTION. Three circles of
// radius 0.25 m in the world's XY, YZ and ZX planes read as a ball from any side
// and point nowhere, where the sun's arrow (3d/sun_marker.h) would say a way it
// does not shine. They are 3d/outline.h's quads, centred on each edge as the
// sun's are: a fixed pixel width from each end's own depth, wound to face the
// eye, in the caller's arena (rule 11). An edge seen exactly end-on is left out.
//
// WORLD AXES, because a lamp's rotation and scale change nothing it lights
// (scene/point_light_component.h): only its world position places the marker,
// so a lamp turned or scaled to nothing is still seen and picked.
//
// A CUBE IS HIT AND NOT THE LINES, as the sun's is: thin lines are a few pixels
// a person would have to land on exactly, and a world-axis cube of half extent
// 0.25 m about the position is the circles' reach.
#pragma once

#include <3d/outline.h>
#include <3d/pick.h>

#include <base/arena.h>

#include <math/double3.h>

#include <render/device.h>

// The marker's edges — three circles of 12 — and the vertices and indices they
// come to, four and six an edge.
#define VOE_3D_POINT_LIGHT_MARKER_CIRCLE 12
#define VOE_3D_POINT_LIGHT_MARKER_EDGES (3 * VOE_3D_POINT_LIGHT_MARKER_CIRCLE)
#define VOE_3D_POINT_LIGHT_MARKER_VERTICES (VOE_3D_POINT_LIGHT_MARKER_EDGES * 4)
#define VOE_3D_POINT_LIGHT_MARKER_INDICES (VOE_3D_POINT_LIGHT_MARKER_EDGES * 6)

// Builds the marker of a point light at world `position` as seen through `view`
// from `eye`, in metres about `eye` in float (ADR-0250), `pixels` wide on a
// picture `size` pixels big, into `arena`, and fills `out`. The XY circle's
// edges come first, then the YZ circle's, then the ZX circle's.
//
// False, with `out` untouched, when `size` has no area.
[[nodiscard]] bool voe_3d_point_light_marker_quads(voe_math_double3 position,
						   voe_render_view view,
						   voe_math_double3 eye,
						   voe_platform_size size,
						   float pixels,
						   voe_base_arena *arena,
						   voe_3d_outline_mesh *out);

// Whether `ray` meets the cube of a point light at world `position`, and how far
// along it: `distance`, which may be NULL, is where it enters the cube, or where
// it leaves it when the ray starts inside, in metres along the world ray as
// voe_3d_sun_marker_hit's is. False for no hit in front of the ray's origin.
[[nodiscard]] bool voe_3d_point_light_marker_hit(voe_math_double3 position,
						 voe_3d_ray ray,
						 float *distance);
