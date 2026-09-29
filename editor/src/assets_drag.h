// What a held model or prefab row does (0277 point 8, 0283 point 7): a press
// on a `.glb` or `.prefab` row in the Assets panel starts a drag holding its project-relative path, and where the
// button comes up decides what happens. main.c calls it once a frame, beside
// voe_editor_pick_read and with the same `blocked`, and while it holds, the
// pick is blocked too:
//
//     voe_editor_assets_drag_read(&drag, &session, &undo, &scene, &views,
//                                 &roots[0], &bar, &geometries, models,
//                                 pointer, left && over, blocked);
//
// THE PRESS IS THE PANEL'S. `ui` says which row the pointer went down on and
// still holds (assets_panel.h's `held`, read after the frame), so a press that
// started anywhere else never becomes a drag, and the row is drawn held by
// `ui` for as long as the button is down.
//
// FOUR OUTCOMES, AT THE RELEASE.
// - Over a scene view: a new thing named after the file, wearing it, at the
//   point below; it is selected, and it is one undo step and unsaved, marked
//   the way Add entity marks them.
// - Over the Inspector while the selected thing has a model: its path is
//   replaced through voe_3d_model_submit, one undo step and unsaved, the way
//   an Inspector edit marks them.
// - A `.prefab` row over a scene view: a placed copy's root at the same point,
//   selected, one undo step and unsaved as a model's is (0283 point 7); the
//   world step expands it the next frame. Over the Inspector, nothing.
// - Anywhere else, or while `blocked`: nothing.
//
// THE POINT is what the view's pick ray meets first; else where it crosses
// y = 0 in front of the eye, because a person dropping onto empty space in a
// scene with a floor means the floor; else, looking up or level, 10 m along
// the ray, near enough to be seen and far enough to be out of the camera.
//
// THE INSPECTOR DROP REPLACES THE PATH rather than adding a component: an
// entity has at most one row of a type, so a second model is not a thing to
// add, and swapping what a placed thing wears is what dropping onto its panel
// is for. A thing with no model is left alone; Add component gives it one. A
// prefab's part is left alone too: it is not the person's to edit (0283).
//
// CONSTRAINTS. The Inspector's rectangle is the dock tree laid out below the
// bar, as resize.c lays it out, not what `ui` drew, because the frame's nodes
// are gone by the time this runs. A drop and Ctrl+D on the same frame would
// give the two new things one id (entities.h); nothing guards that pair.
#pragma once

#include "dock.h"
#include "session.h"
#include "topbar.h"
#include "undo.h"
#include "view.h"

#include <3d/model_component.h>
#include <3d/models.h>
#include <3d/shape_geometry.h>

#include <math/float2.h>

#include <scene/prefab_component.h>

#include <stdbool.h>

// The larger of a model row's and a prefab row's path.
#define VOE_EDITOR_ASSETS_DRAG_PATH                    \
	(VOE_3D_MODEL_PATH > VOE_SCENE_PREFAB_PATH ? \
		 VOE_3D_MODEL_PATH :                  \
		 VOE_SCENE_PREFAB_PATH)

// What a drag remembers between frames. Zeroed is no drag.
typedef struct {
	bool holding;
	// Whether the held row is a prefab rather than a model.
	bool prefab;
	// The held row's path, `Assets/...` with `/` separators.
	char path[VOE_EDITOR_ASSETS_DRAG_PATH];
} voe_editor_assets_drag;

// This frame's button against the drag: a start from the Assets panel's held
// model or prefab row, or at the release one of the four outcomes above. `pointer` is
// in `root`'s millimetres and `down` is its primary button.
void voe_editor_assets_drag_read(
	voe_editor_assets_drag *drag, voe_editor_session *session,
	voe_editor_undo *undo, voe_editor_scene *scene,
	const voe_editor_views *views, const voe_editor_dock_root *root,
	const voe_editor_topbar *bar, const voe_3d_shape_geometries *geometries,
	const voe_3d_models *models, voe_math_float2 pointer, bool down,
	bool blocked);
