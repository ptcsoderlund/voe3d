// Snap as arithmetic (ADR-0482, point 4): the three roundings a snapping drag
// applies to the world values it writes, a moved position, a swept angle and a
// scale. Values and a step in, the rounded value out; nothing here knows
// whether snapping is on, which key is held or what the steps are, which are
// the editor's.
//
//     voe_math_double3 at = voe_3d_gizmo_snap_position(dragged, handle, 0.5);
//     float sweep = voe_3d_gizmo_snap_angle(now - press, 15.0f * PI / 180.0f);
//     voe_math_float3 scale = voe_3d_gizmo_snap_scale(scaled, handle, 0.1f);
//
// A handle reads as the axes it names: X, Y, Z alone, XY, YZ, ZX their two,
// CENTRE all three. A step at or below nought, or one not finite, and NONE
// where a handle is asked, are the caller's bug and assert (rule 13).
//
// A POSITION LANDS ON THE WORLD GRID, NOT ON STEPS FROM THE PRESS (ADR-0438).
// Steps from the press keep an object's odd offset, so two things placed apart
// never line up; rounding the world value puts both on one grid. The axes the
// handle does not drag are left exactly as they were, so a move along X never
// pulls a Y that was off the grid onto it. The rounding is in double, so a
// step stays exact 100 km out.
//
// A TURN IS WHOLE STEPS FROM THE PRESS. A rotation about a world axis has no
// world grid to land on: the entity's own turn is any quaternion, and the ring
// adds to it. What is snapped is the angle swept since the press, so the turn
// written is the press's times whole steps. The sweep is wrapped to (−π, π]
// first, because a ring's angle now less its angle at the press can be a whole
// turn off the short way round, and that sweep snaps as the short one.
//
// A SCALE NEVER REACHES NOUGHT. A component rounded to nought flattens the
// entity to nothing, which the next drag cannot grow back (its factor times
// nought) and whose normals have no inverse to come from. So a component's
// size is never under one step; its sign is kept, so a mirrored axis stays
// mirrored. Snapping rounds world values, so a parented child's own row may
// read off the step.
//
// THE INSPECTOR NEVER CALLS THIS. A typed value is what the person typed and
// is written as it is (ADR-0438); snapping is for the drags alone.
#pragma once

#include <3d/gizmo.h>

#include <math/double3.h>
#include <math/float3.h>

// `position` with each world axis `handle` drags rounded to the nearest
// multiple of `step`, in metres, the rest untouched.
voe_math_double3 voe_3d_gizmo_snap_position(voe_math_double3 position,
					    voe_3d_gizmo_handle handle,
					    double step);

// `angle` wrapped to (−π, π], then rounded to the nearest multiple of `step`;
// both in radians.
float voe_3d_gizmo_snap_angle(float angle, float step);

// `scale` with each component `handle` scales rounded to the nearest multiple
// of `step`, its size never under one step and its sign kept, the rest
// untouched.
voe_math_float3 voe_3d_gizmo_snap_scale(voe_math_float3 scale,
					voe_3d_gizmo_handle handle, float step);
