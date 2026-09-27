// A sun drawn in an editor's view as a marker (0274): a circle and an arrow
// along the way it shines, as line quads about the eye, and the ray that picks
// it. It builds two arrays in an arena and uploads nothing; the draw system is
// what hands them to the card.
//
//     voe_3d_outline_mesh mesh;
//
//     if (voe_3d_sun_marker_quads(pose, view, eye, size, 2.0f, arena, &mesh))
//             ... // mesh.vertices, mesh.indices: this frame's geometry
//     if (voe_3d_sun_marker_hit(pose, ray, &distance))
//             ... // the sun is under the pointer
//
// THE GEOMETRY IS 0274's. A circle of radius 0.25 m in the sun's own XY plane
// and an arrow from its centre 1 m along its -Z, the way it shines
// (scene/light_component.h), with a head of four short edges.
//
// LINES AND NOT A SOLID, because a sun has no body: what it is is a direction,
// and lines show the direction without hiding what lies behind the marker. They
// are 3d/outline.h's quads, centred on each edge as the camera marker's are
// (3d/camera_marker.h): a fixed pixel width from each end's own depth, wound to
// face the eye, in the caller's arena (rule 11). An edge seen exactly end-on —
// the arrow's shaft when the eye looks straight along it — is left out.
//
// SCALE IS IGNORED: only position and rotation place the marker. A sun's scale
// changes nothing it lights, so a sun scaled to nothing still shines and must
// still be seen and picked.
//
// A CUBE IS HIT AND NOT THE LINES. Thin lines are a few pixels a person would
// have to land on exactly; a cube of half extent 0.25 m about the position, in
// the sun's own axes, is the circle's reach and picks as the camera's box does.
#pragma once

#include <3d/outline.h>
#include <3d/pick.h>

#include <base/arena.h>

#include <math/double3.h>

#include <render/device.h>

#include <scene/transform_component.h>

// The marker's edges — a circle of 24, the arrow's shaft and its head's four —
// and the vertices and indices they come to, four and six an edge.
#define VOE_3D_SUN_MARKER_CIRCLE 24
#define VOE_3D_SUN_MARKER_EDGES (VOE_3D_SUN_MARKER_CIRCLE + 1 + 4)
#define VOE_3D_SUN_MARKER_VERTICES (VOE_3D_SUN_MARKER_EDGES * 4)
#define VOE_3D_SUN_MARKER_INDICES (VOE_3D_SUN_MARKER_EDGES * 6)

// Builds the marker of a sun at `pose` as seen through `view` from `eye`, in
// metres about `eye` in float (ADR-0250), `pixels` wide on a picture `size`
// pixels big, into `arena`, and fills `out`. The circle's edges come first,
// then the shaft, then the head.
//
// False, with `out` untouched, when `size` has no area.
[[nodiscard]] bool voe_3d_sun_marker_quads(voe_scene_transform pose,
					   voe_render_view view,
					   voe_math_double3 eye,
					   voe_platform_size size, float pixels,
					   voe_base_arena *arena,
					   voe_3d_outline_mesh *out);

// Whether `ray` meets the cube of a sun at `pose`, and how far along it:
// `distance`, which may be NULL, is where it enters the cube, or where it
// leaves it when the ray starts inside, in metres along the world ray as
// voe_3d_camera_marker_hit's is. False for no hit in front of the ray's origin.
[[nodiscard]] bool voe_3d_sun_marker_hit(voe_scene_transform pose,
					 voe_3d_ray ray, float *distance);
