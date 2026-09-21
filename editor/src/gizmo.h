// What the primary button does to the selected entity's move gizmo: which handle
// the pointer is over, a press on one grabbing it, and a drag submitting the
// entity's new position until the release. The gizmo, what a ray meets and where
// it grabs are `3d`'s (3d/gizmo.h, ADR-0205); what is here is the pointer, the
// button and the entity. It is asked before pick.h each frame, so a press this
// file takes is one picking never sees (voe_editor_gizmo_taking).
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
// Each frame the ray is met against the handle taken through `start`, and the
// entity is put at `start` plus how far that point has travelled from `grab`.
// Measured from the moving position, each frame would add a small error to the
// last and the entity would creep away from under the pointer; measured from
// the press, the same pointer is always the same position, and a submitted
// position that has not been drained yet changes nothing.
//
// THE AXES ARE THE WORLD'S. A gizmo that turns with its entity is a later toggle
// (`feature.md`); it changes the three directions 3d/gizmo.h hands out and
// nothing here.
//
// A WHOLE TRANSFORM IS SUBMITTED, NOT A DELTA, AND IT LANDS A FRAME LATER. The
// row is read as it is, its position replaced, and the result submitted to the
// transform system, which is the one writer of that table (rule 3, rule 4,
// scene/transform_system.h). Rotation and scale go back as they were read, and
// two submitters in one frame resolve as last-writer-wins.
//
// A ZEROED STRUCT HOLDS NOTHING. VOE_3D_GIZMO_NONE is nought, so a zeroed
// gizmo has no handle held and none hovered, and `captured` is read only while
// something is held, so its nought names no view.
//
// `moved` IS WHAT TELLS THE CALLER AN EDIT REACHED THE PROJECT: the moves
// submitted in the last read, each one a position that actually changed. A drag
// held still submits nothing and counts nothing.
#pragma once

#include "scene.h"
#include "view.h"

#include <3d/gizmo.h>

#include <math/float2.h>
#include <math/float3.h>

#include <stdbool.h>
#include <stdint.h>

// What the gizmo remembers between frames.
typedef struct {
	bool pointer_was_down;
	voe_3d_gizmo_handle held; // NONE when no drag is running
	voe_3d_gizmo_handle hovered; // what the pointer is over this frame
	uint32_t captured; // the view the drag started in; read only while held
	uint32_t hovered_view;
	voe_math_float3 grab; // where on the handle the press landed
	voe_math_float3 start; // the entity's position then
	uint32_t moved; // moves submitted this frame; zeroed every read
} voe_editor_gizmo;

// This frame's pointer and primary button, once a frame at pick.h's place and
// before it. `pointer` is in the root surface's millimetres, `pixels` is the
// shaft's length on the picture — the number the pass is handed, so what is hit
// is what is drawn — and `blocked` is pick.h's.
void voe_editor_gizmo_read(voe_editor_gizmo *gizmo, voe_editor_scene *scene,
			   const voe_editor_views *views, float pixels,
			   voe_math_float2 pointer, bool down, bool blocked);

// True from a press that grabbed a handle until its release: the press is this
// file's and not picking's.
bool voe_editor_gizmo_taking(const voe_editor_gizmo *gizmo);

// The handle `view` shows marked: the held one in the captured view, the hovered
// one in the hovered view while nothing is held, NONE everywhere else.
voe_3d_gizmo_handle voe_editor_gizmo_marked(const voe_editor_gizmo *gizmo,
					    uint32_t view);
