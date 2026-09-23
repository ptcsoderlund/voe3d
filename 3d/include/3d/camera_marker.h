// A scene camera drawn in an editor's view as a marker (0223): every edge of a
// box and of a frustum as line quads standing in the world, and the ray that
// picks the box. It builds two arrays in an arena and uploads nothing; the
// draw system is what hands them to the card.
//
//     voe_3d_outline_mesh mesh;
//
//     if (voe_3d_camera_marker_quads(pose, lens, view, size, 2.0f, arena,
//                                    &mesh))
//             ... // mesh.vertices, mesh.indices: this frame's geometry
//     if (voe_3d_camera_marker_hit(pose, ray, &distance))
//             ... // the camera is under the pointer
//
// THE GEOMETRY IS 0223's. A box of half extents 0.1, 0.075 and 0.15 m centred
// on the camera, and a frustum from the camera's origin to 1 m ahead along -Z
// at the lens's fov_y and an aspect of 16:9 — the game window's aspect is not
// known to the editor, and 16:9 is the common screen. All of it is in the
// camera's own space under its whole transform matrix, so the marker turns,
// rolls and scales with the camera, and a camera scaled to nothing, whose
// matrix has no inverse, has no marker and cannot be picked.
//
// THE LINES ARE 3d/outline.h's QUADS: a fixed pixel width worked out per vertex
// from that vertex's own depth, each quad extended half a width past both ends
// so corners have no notch, wound to face the eye with normals pointing at it,
// and everything handed back in the caller's arena (rule 11). A marker edge is
// a line and not a fold, so its quad is centred on the edge, a half width to
// each side, where an outline's stands outside the surface. The arithmetic
// mirrors outline.c's, which keeps its helpers to itself.
//
// ONLY THE BOX IS HIT. A click beside the camera must not select it through its
// thin frustum lines, so the lines are drawn and never picked.
#pragma once

#include <3d/outline.h>
#include <3d/pick.h>

#include <base/arena.h>

#include <render/device.h>

#include <scene/camera_component.h>
#include <scene/transform_component.h>

// The marker's edges — the box's twelve and the frustum's eight: four from the
// origin to the far corners and four around the far rectangle — and the
// vertices and indices they come to, four and six an edge.
#define VOE_3D_CAMERA_MARKER_EDGES (12 + 8)
#define VOE_3D_CAMERA_MARKER_VERTICES (VOE_3D_CAMERA_MARKER_EDGES * 4)
#define VOE_3D_CAMERA_MARKER_INDICES (VOE_3D_CAMERA_MARKER_EDGES * 6)

// Builds the marker of a camera at `pose` with `lens` as seen through `view`,
// `pixels` wide on a picture `size` pixels big, into `arena`, and fills `out`.
// An edge seen exactly end-on is left out, as outline.c leaves one out.
//
// False, with `out` untouched, when the pose's matrix has no inverse (a camera
// scaled to nothing) or `size` has no area.
[[nodiscard]] bool voe_3d_camera_marker_quads(voe_scene_transform pose,
					      voe_scene_camera lens,
					      voe_render_view view,
					      voe_platform_size size,
					      float pixels,
					      voe_base_arena *arena,
					      voe_3d_outline_mesh *out);

// Whether `ray` meets the box of a camera at `pose`, and how far along it.
//
// The ray is taken into the camera's space by the inverse of its matrix, the
// direction not renormalised, so the slab test's parameter is still a distance
// along the world ray: `distance`, which may be NULL, is where it enters the
// box, or where it leaves it when the ray starts inside.
//
// False for no hit in front of the ray's origin, or a pose with no inverse.
[[nodiscard]] bool voe_3d_camera_marker_hit(voe_scene_transform pose,
					    voe_3d_ray ray, float *distance);
