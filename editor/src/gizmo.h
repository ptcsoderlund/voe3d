// What the primary button does to the selected entity's gizmo, arrows that move
// it, rings that turn it or scales that scale it, by scene.h's `gizmo_kind`:
// which handle the pointer is over, a press on one grabbing it, and a drag
// submitting the entity's new position, rotation or scale until the release.
// The gizmo, what a ray meets and where it grabs are `3d`'s (3d/gizmo.h,
// 3d/gizmo_rings.h, 3d/gizmo_scales.h, ADR-0205, 0274, 0482); what is here is
// the pointer, the button and the entity. It is asked before pick.h each frame,
// so a press this file takes is one picking never sees (voe_editor_gizmo_taking).
// A switch of mode while a handle is held ends the drag.
// A prefab's part is never hovered or grabbed: it has no gizmo (0283 point 5).
//
// THE MIDDLE BUTTON IS NEVER READ IN HERE, for pick.h's reason. The middle
// button is the views' camera and the left is the interface's, and turning the
// camera about a scene must never move what is in it: a person orbiting is
// looking, not editing. So this file reads the primary button only.
//
// `blocked` IS pick.h's `blocked`: the browser, Preferences or the colour picker
// over the views. While it holds there is no hover, no press is taken and a
// running drag ends, and the button is still remembered, so a press that
// started over one of them grabs nothing when the pointer reaches a handle.
//
// THE DRAG IS MEASURED FROM THE PRESS AND NOT FROM WHERE THE ENTITY HAS GOT TO.
// Each frame the ray is met against the handle taken through `start`. A move
// puts the entity at `start` plus how far that point has travelled from `grab`;
// a turn gives it `kept`, the rotation at the press, turned about the ring's
// world axis by the angle now less `grab_angle`; a scale gives it `scale_start`
// times voe_3d_gizmo_scales_factor of `grab` and now, on the held axis or all
// three for the centre. Measured from where it has got
// to, each frame would add a small error to the last and the entity would creep
// away from under the pointer; measured from the press, the same pointer is
// always the same pose, and a submission not yet drained changes nothing. The
// points are double, world positions (ADR-0250), so 100 km out keeps its mm.
//
// THE ARROWS' AND RINGS' AXES ARE THE WORLD'S; THE SCALES' ARE THE ENTITY'S OWN
// (0482 point 1), its world rotation handed in as `turn`: at the press, `kept`.
//
// SNAP: a frame of a drag snaps when `scene->snap.on != flip`, Ctrl's level read
// every frame (0482 point 6). What is rounded is the world value the drag writes,
// by 3d/gizmo_snap.h (0482 point 4): the position on the world grid along the
// held handle's axes, the swept angle in whole steps from the press, the scale
// on multiples. A typed Inspector value never comes here.
//
// A WHOLE TRANSFORM IS SUBMITTED, NOT A DELTA, AND IT LANDS A FRAME LATER. The
// drag is in world space: the entity's world transform, composed up its parents,
// has its position, rotation or scale replaced, and is then made the row relative to
// the parent (ADR-0271) and submitted to the transform system, the one writer of
// that table (rule 3, rule 4, scene/transform_system.h). Children keep their
// rows, so a turned turret carries its barrel. Two submitters in one frame
// resolve as last-writer-wins.
//
// A ZEROED STRUCT HOLDS NOTHING. VOE_3D_GIZMO_NONE is nought, so a zeroed
// gizmo has no handle held and none hovered, and `captured` is read only while
// something is held, so its nought names no view.
//
// `moved` IS WHAT TELLS THE CALLER AN EDIT REACHED THE PROJECT: the moves, turns
// and scales submitted in the last read, each a position, rotation or scale that
// actually changed, so a drag marks unsaved and settles into one undo step
// (ADR-0204). A drag held still, or a refused grab, submits nothing and counts
// nothing.
#pragma once

#include "scene.h"
#include "view.h"

#include <3d/gizmo.h>

#include <math/double3.h>
#include <math/float2.h>
#include <math/float3.h>
#include <math/quat.h>

#include <stdbool.h>
#include <stdint.h>

// What the gizmo remembers between frames.
typedef struct {
	bool pointer_was_down;
	voe_3d_gizmo_handle held; // NONE when no drag is running
	voe_3d_gizmo_handle hovered; // what the pointer is over this frame
	uint32_t captured; // the view the drag started in; read only while held
	uint32_t hovered_view;
	voe_math_double3 grab; // where on the handle the press landed
	voe_math_double3 start; // the entity's world position then
	voe_3d_gizmo_kind kind; // the mode the held drag began in; read only while held
	float grab_angle; // where about the ring's axis the press landed
	voe_math_quat kept; // the entity's world rotation then
	voe_math_float3 scale_start; // the entity's world scale then
	uint32_t moved; // moves, turns and scales submitted this frame; zeroed every read
} voe_editor_gizmo;

// This frame's pointer and primary button, once a frame at pick.h's place and
// before it. `pointer` is in the root surface's millimetres, `pixels` is the
// shaft's length on the picture — the number the pass is handed, so what is hit
// is what is drawn — `blocked` is pick.h's, and `flip` is Ctrl's level this
// frame, flipping snap.
void voe_editor_gizmo_read(voe_editor_gizmo *gizmo, voe_editor_scene *scene,
			   const voe_editor_views *views, float pixels,
			   voe_math_float2 pointer, bool down, bool blocked,
			   bool flip);

// True from a press that grabbed a handle until its release: the press is this
// file's and not picking's.
bool voe_editor_gizmo_taking(const voe_editor_gizmo *gizmo);

// The handle `view` shows marked: the held one in the captured view, the hovered
// one in the hovered view while nothing is held, NONE everywhere else.
voe_3d_gizmo_handle voe_editor_gizmo_marked(const voe_editor_gizmo *gizmo,
					    uint32_t view);
