// The rotate gizmo as arithmetic (ADR-0274): three rings about the world's X, Y
// and Z through a gizmo's origin, radius one shaft, which ring a ray meets, the
// angle about its axis a ray points at, and this frame's triangles. It is the
// move gizmo's struct and handles (3d/gizmo.h) — X, Y and Z name the ring about
// that axis — and like it, nothing here knows what a selection or a pointer is.
//
//     voe_3d_gizmo gizmo = voe_3d_gizmo_at(position, view, eye, size, 90.0f);
//     voe_3d_gizmo_handle under = voe_3d_gizmo_rings_hit(gizmo, ray);
//     float angle;
//
//     if (voe_3d_gizmo_rings_angle(gizmo, under, ray, &angle))
//             ... // the turn so far is `angle` now minus `angle` at the press
//     if (voe_3d_gizmo_rings_quads(gizmo, under, arena, &plain, &marked))
//             ... // two meshes, two draws, as the move gizmo's
//
// THE AXES ARE THE WORLD'S, for 0205's reason: the rings sit on the move
// gizmo's world axes, and an entity's own axes are the later toggle both headers
// name, which changes these three directions and nothing else.
//
// THE ANGLE IS MEASURED AND NOT A DELTA. The call answers where about the axis
// the ray points, in radians, right-handed, from the ring's first in-plane axis
// (Y about X, Z about Y, X about Z). A drag is that angle now against that angle
// at the press, as a move drag is a point now against a point then: a
// frame-to-frame delta would sum each frame's float error into the rotation and
// drift, where a difference of two measurements cannot.
//
// A RING IS QUADS FACING THE EYE, for the reason a move gizmo's arrow is one
// (3d/gizmo.h): one unlit record, every ring as readable from one side as from
// another, and a ring seen edge on a line and never nothing. Each of its
// segments is widened across itself towards the eye.
//
// Positions are double and the arithmetic float about one point (ADR-0250): a
// hit or an angle about the ray's origin, the quads about the eye.
#pragma once

#include <3d/gizmo.h>
#include <3d/pick.h>

#include <base/arena.h>

#include <stdbool.h>

// How many straight segments one ring is drawn as: at a gizmo's size on screen
// a 7.5 degree corner reads as round.
#define VOE_3D_GIZMO_RING_SEGMENTS 48

// What the two meshes cost together, whatever is marked: a quad per segment of
// three rings. A program sizes its transient vertices and indices from these.
#define VOE_3D_GIZMO_RING_VERTICES (3 * VOE_3D_GIZMO_RING_SEGMENTS * 4)
#define VOE_3D_GIZMO_RING_INDICES (3 * VOE_3D_GIZMO_RING_SEGMENTS * 6)

// Which ring `ray` meets: where it crosses that ring's plane in front of the
// eye, within VOE_3D_GIZMO_GRIP shafts of the radius, nearest along the ray
// first — a ring in front hides the one behind it, as a person sees it.
// VOE_3D_GIZMO_NONE for none, for a ray in a ring's plane, for a gizmo behind
// the eye and for a shaft of nought.
voe_3d_gizmo_handle voe_3d_gizmo_rings_hit(voe_3d_gizmo gizmo, voe_3d_ray ray);

// The angle about `handle`'s axis, in radians in (-pi, pi], of where `ray`
// crosses that ring's plane, measured about the gizmo's origin. False with
// `*out` untouched for VOE_3D_GIZMO_NONE, a plane handle, or a ray too nearly in
// the plane to cross it anywhere in particular — an ordinary frame the caller
// answers by keeping the rotation it had.
[[nodiscard]] bool voe_3d_gizmo_rings_angle(voe_3d_gizmo gizmo,
					    voe_3d_gizmo_handle handle,
					    voe_3d_ray ray, float *out);

// This frame's triangles in `arena`, in metres about `gizmo.eye`, as
// voe_3d_gizmo_quads builds its own: every ring but `marked` into `plain`, and
// `marked`'s ring into `marked_out` at VOE_3D_GIZMO_MARKED_STEP times the
// width; NONE or a plane handle leaves `marked_out` empty. Every normal points
// at the eye and every triangle is wound to face it. False, with both
// untouched, for a gizmo of no shaft.
[[nodiscard]] bool voe_3d_gizmo_rings_quads(voe_3d_gizmo gizmo,
					    voe_3d_gizmo_handle marked,
					    voe_base_arena *arena,
					    voe_3d_gizmo_mesh *plain,
					    voe_3d_gizmo_mesh *marked_out);
