// A left click in a scene view selects the frontmost entity under the pointer,
// and a click on nothing clears the selection — the same selection a row in
// `Scene` moves (scene.h). Every placed thing is picked (0354): one with a mesh
// on it — a shape, a model on its loaded triangles (ADR-0277), water — and
// every other on its marker, and a marker under the pointer beats a mesh
// whichever is nearer, so a lamp inside a house is still clicked. A blocker
// is picked by its marker, never inside its box. The ray, and what it meets, are `3d`'s
// (3d/pick.h, ADR-0202); what is here is the press edge and which view was
// clicked.
//
// THE MIDDLE BUTTON IS NEVER READ IN HERE. The middle button is the views'
// camera and the left is the interface's, which is main.c's standing division
// of the mouse, and an orbit must never change what is selected: a person
// turning the camera about a scene is looking, not picking. So this file reads
// one button, the primary one, and voe_editor_views_drag reads the other; the
// two never compete for a press and neither has to undo the other.
//
// WHAT `blocked` IS FOR A CALLER. It is the frames where the press is not the
// views' to take: the file browser showing, Preferences showing, or the colour
// picker open. Each of them paints over the views or takes the press for
// itself, and `feature.md` asks for the first two by name — a click in a view
// behind the browser selects nothing. A caller with a fourth such panel one day
// adds it to the same `||`, because this file knows nothing about panels.
//
// A PRESS SELECTS, NOT A RELEASE. It is where `ui`'s own widgets act and where
// the Scene panel's rows do (scene.h), so a click in a view lands at the same
// moment as a click on a row and the two feel like one interface. It is also
// what lets a press that was refused — `blocked` — never turn into a selection
// later: the button is remembered on every call, blocked or not, so a press
// that started over the browser and was released over a view finds no edge.
//
// A CLICK OVER NO VIEW AT ALL LEAVES THE SELECTION ALONE; A CLICK IN A VIEW
// THAT MEETS NOTHING CLEARS IT. Empty space in a view is an answer — there is
// nothing there and the person pointed at it — while the rest of the editor is
// not: pressing a button in the bar, a row in `Scene` or a field in the
// Inspector says nothing about what is selected, and would be a maddening way
// to lose a selection.
#pragma once

#include "scene.h"
#include "view.h"

#include <3d/models.h>
#include <3d/shape_geometry.h>

#include <math/float2.h>

// What picking remembers between frames.
typedef struct {
	// Last frame's primary button, so a press is found as an edge.
	bool pointer_was_down;
} voe_editor_pick;

// This frame's press, once a frame, beside voe_editor_views_drag. `pointer` is
// in the root surface's millimetres and `down` is its primary button; `models`
// is the store model rows are picked through, NULL for none; `blocked`
// says the press is not the views' to take. Does nothing at all while `blocked`
// and still remembers the button, so a press that started over something else
// never selects when it is released over a view.
void voe_editor_pick_read(voe_editor_pick *pick, voe_editor_scene *scene,
			  const voe_editor_views *views,
			  const voe_3d_shape_geometries *geometries,
			  const voe_3d_models *models,
			  voe_math_float2 pointer, bool down, bool blocked);
