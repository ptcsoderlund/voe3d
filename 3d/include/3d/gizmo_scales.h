// The scale gizmo as arithmetic (ADR-0482): three shafts along the entity's own
// axes, each ending in a square, and a square at the centre; which handle a ray
// meets, where a drag starts, the factor it has come to and this frame's
// triangles. It is the move gizmo's struct and handles (3d/gizmo.h), X, Y, Z and
// VOE_3D_GIZMO_CENTRE, and like it knows no selection and no pointer. `turn` is
// the entity's world rotation; its axes are voe_math_float4x4_from_quat(turn)'s
// columns.
//
//     voe_3d_gizmo_handle under = voe_3d_gizmo_scales_hit(gizmo, turn, ray);
//     voe_math_double3 press, now;
//     if (voe_3d_gizmo_scales_grab(gizmo, turn, under, ray, &press))
//             ... // later, grabbed again into `now`:
//     float factor = voe_3d_gizmo_scales_factor(gizmo, turn, under, press, now);
//
// THE AXES ARE THE ENTITY'S OWN. A scale is per the entity's own axis; a world
// axis on a turned box would stretch it along some line other than the one
// dragged. The arrows and rings stay on the world's.
//
// THE FACTOR IS MEASURED FROM THE PRESS, as a move is a point now against a
// point then (ADR-0205): the caller multiplies the world scale it had at the
// press, and a factor from frame to frame would creep.
//
// THE CENTRE READS TRAVEL UP THE PLANE FACING THE EYE. A press on the centre has
// no distance from the centre to shrink from, so its factor is 1 plus the travel
// along that plane's direction nearest the world's up, in shafts: up grows, down
// shrinks. Looking straight along Y, the direction nearest world -Z is taken.
//
// THE FACTOR HAS A FLOOR OF VOE_3D_GIZMO_SCALE_LEAST. A drag through the centre
// or past it would read nought or less, a scale that flattens or mirrors; a
// hundredth is as small as a drag takes it and can be dragged back from.
//
// THE CENTRE SQUARE IS MET BEFORE ANY AXIS. It is where the shafts start, so on
// the picture they run through it; a press inside it is the centre, and an axis
// is met only beyond it, nearest along the ray. An axis seen end on lies inside
// it and is the centre too, which it has to be: such an axis has no drag.
//
// Every handle is quads facing the eye, for 3d/gizmo.h's reason, and the squares
// stand upright against the world's up. No labels. Positions are double and the
// arithmetic float about one point (ADR-0250): a hit, grab or factor about the
// ray's origin or the gizmo's, the quads about the eye.
#pragma once

#include <3d/gizmo.h>
#include <3d/pick.h>

#include <base/arena.h>

#include <math/double3.h>
#include <math/quat.h>

#include <stdbool.h>

// Half a tip square's side, a fraction of a shaft: inside VOE_3D_GIZMO_GRIP, so
// all of a square drawn at a tip is met as its axis.
#define VOE_3D_GIZMO_SCALE_TIP_HALF 0.08f
// Half the centre square's side, a fraction of a shaft: half again a tip's, so
// the one handle that scales everything reads as the bigger target.
#define VOE_3D_GIZMO_SCALE_CENTRE_HALF 0.12f
// The least factor a drag answers.
#define VOE_3D_GIZMO_SCALE_LEAST 0.01f

// What the two meshes cost together, whatever is marked: three shaft quads,
// three tip squares and the centre square. A program sizes its transient
// vertices and indices from these.
#define VOE_3D_GIZMO_SCALE_VERTICES (3 * 4 + 3 * 4 + 4)
#define VOE_3D_GIZMO_SCALE_INDICES (3 * 6 + 3 * 6 + 6)

// Which handle `ray` meets: CENTRE where it crosses the centre square in front
// of the eye; else the axis it passes within VOE_3D_GIZMO_GRIP shafts of, from
// the origin to its tip square's far side, nearest along the ray. NONE for none,
// for a gizmo behind the eye and for a shaft of nought, as the rings' hit.
voe_3d_gizmo_handle voe_3d_gizmo_scales_hit(voe_3d_gizmo gizmo,
					    voe_math_quat turn, voe_3d_ray ray);

// Where on `handle` this ray lands, in world metres: for an axis the point of
// its line nearest the ray, for the centre where the ray meets the plane through
// the origin facing the eye. False with `*out` untouched for NONE, a plane
// handle, or a ray too nearly along the axis or in the plane, as
// voe_3d_gizmo_grab: an ordinary frame the caller answers by keeping its scale.
[[nodiscard]] bool voe_3d_gizmo_scales_grab(voe_3d_gizmo gizmo,
					    voe_math_quat turn,
					    voe_3d_gizmo_handle handle,
					    voe_3d_ray ray,
					    voe_math_double3 *out);

// The factor a drag from `grab` to `now`, both grabs of `handle`, has come to:
// for an axis the signed distance along it from the origin now over that at the
// press, for the centre 1 plus the travel up the plane facing the eye in
// shafts; never under VOE_3D_GIZMO_SCALE_LEAST. 1 for NONE or a plane handle,
// for a shaft of nought, and for an axis pressed within a thousandth of a shaft
// of the origin, which has nothing to scale from.
float voe_3d_gizmo_scales_factor(voe_3d_gizmo gizmo, voe_math_quat turn,
				 voe_3d_gizmo_handle handle,
				 voe_math_double3 grab, voe_math_double3 now);

// This frame's triangles in `arena`, in metres about `gizmo.eye`, as
// voe_3d_gizmo_quads builds its own: every handle but `marked` into `plain`,
// and `marked`'s shaft and tip, or the centre, into `marked_out` at
// VOE_3D_GIZMO_MARKED_STEP times the width; NONE or a plane handle leaves
// `marked_out` empty. Every normal points at the eye and every triangle is
// wound to face it. False, with both untouched, for a gizmo of no shaft.
[[nodiscard]] bool voe_3d_gizmo_scales_quads(voe_3d_gizmo gizmo,
					     voe_math_quat turn,
					     voe_3d_gizmo_handle marked,
					     voe_base_arena *arena,
					     voe_3d_gizmo_mesh *plain,
					     voe_3d_gizmo_mesh *marked_out);
