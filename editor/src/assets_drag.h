// What a held Assets row does (0277 point 8, 0283 point 7, 0298 point 5, 0377
// point 3): a press on any row starts a drag holding its project-relative
// path, and where the button comes up decides what happens; a folder or a file
// that only lists only moves. main.c calls it once a frame, beside
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
// THE OUTCOMES, AT THE RELEASE.
// - Over a folder row other than the dragged one, or over Up: the row moves
//   there through voe_editor_assets_move, followed and refused as it says; a
//   row marked dragged (assets_panel.h) enters no folder on the release.
// - A model over a scene view: a new thing named after the file, wearing it,
//   at the point below, selected, one undo step and unsaved as Add entity is;
//   over the Inspector while the selected thing has a model, its path replaced
//   through voe_3d_model_submit, one undo step and unsaved as an edit is.
// - A `.prefab` over a scene view: a placed copy's root at the same point,
//   selected, one step and unsaved (0283 point 7), expanded the next frame;
//   while a prefab is open, nothing.
// - A picture over the Inspector: while a `.material` is open, over a map row
//   (inspector_material.h) that map's path in `scene->material`, which
//   interface_read.c carries to the table; while the selected thing has an
//   emitter, its texture through voe_3d_emitter_submit, one step and unsaved.
// - A `.material` over the Inspector showing a thing (0399 point 8): over its
//   model's Materials n field, `materials[n-1]` through voe_3d_model_submit;
//   elsewhere, a shape's `material` through voe_3d_shape_submit; one step and
//   unsaved as the model swap is.
// - Anywhere else, or while `blocked`: nothing.
// The drag starts past VOE_EDITOR_SCENE_DRAG_START (scene_list.h) from the
// press, so a click opens a prefab or folder and a release before it does
// nothing. Then voe_editor_assets_drag_ghost_draw shows the file's name by the
// pointer, refused (drag_ghost.h) wherever the release would do nothing: over
// the dragged row itself too, or a folder inside a dragged folder.
//
// THE POINT is what the view's pick ray meets first; else where it crosses
// y = 0 in front of the eye, because a person dropping onto empty space in a
// scene with a floor means the floor; else, looking up or level, 10 m along
// the ray, near enough to be seen and far enough to be out of the camera.
//
// THE INSPECTOR DROP REPLACES THE PATH rather than adding a component: an
// entity has at most one row of a type, so a second model is not a thing to
// add, and swapping what a placed thing wears is what dropping onto its panel
// is for. A thing with no model (or emitter, for a picture) is left alone, and
// a prefab's part: not the person's to edit (0283). A path longer than its
// field's room is no drag.
//
// CONSTRAINTS. The Inspector's rectangle is the dock tree laid out below the
// bar, and the panel's rows, Up, the Inspector's fields and the map rows are
// last frame's rectangles, not what `ui`
// drew, because the frame's nodes are gone by the time this runs. A drop and
// Ctrl+D on one frame would give two new things one id (entities.h); nothing
// guards that pair.
#pragma once

#include "dock.h"
#include "drag_ghost.h"
#include "models.h"
#include "session.h"
#include "topbar.h"
#include "undo.h"
#include "view.h"

#include <3d/emitter_component.h>
#include <3d/model_component.h>
#include <3d/models.h>
#include <3d/shape_geometry.h>

#include <math/float2.h>

#include <scene/prefab_component.h>

#include <stdbool.h>

// The larger of two sizes, for the drag's path.
#define VOE_EDITOR_ASSETS_DRAG_LARGER(a, b) ((a) > (b) ? (a) : (b))

// The largest of a model row's, a prefab row's and an emitter's texture path.
#define VOE_EDITOR_ASSETS_DRAG_PATH                                    \
	VOE_EDITOR_ASSETS_DRAG_LARGER(                                 \
		VOE_EDITOR_ASSETS_DRAG_LARGER(VOE_3D_MODEL_PATH,       \
					      VOE_SCENE_PREFAB_PATH), \
		VOE_3D_EMITTER_TEXTURE)

// What a drag remembers between frames. Zeroed is no drag.
typedef struct {
	bool holding;
	// Whether the held row is a prefab, a picture or a `.material` rather
	// than a model, or none of the four, a folder or a file that only
	// moves.
	bool prefab;
	bool picture;
	bool material;
	bool moves_only;
	// The held row's path, `Assets/...` with `/` separators.
	char path[VOE_EDITOR_ASSETS_DRAG_PATH];
	// The pointer at the press.
	voe_math_float2 from;
	// Whether the pointer has moved past the start threshold since.
	bool dragging;
	// Whether a release at this frame's pointer would do nothing.
	bool refused;
} voe_editor_assets_drag;

// This frame's button against the drag: a start from the Assets panel's held
// row, or at the release one of the outcomes above. `pointer` is in `root`'s millimetres and `down` is its primary button.
void voe_editor_assets_drag_read(
	voe_editor_assets_drag *drag, voe_editor_session *session,
	voe_editor_undo *undo, voe_editor_scene *scene,
	const voe_editor_views *views, const voe_editor_dock_root *root,
	const voe_editor_topbar *bar, const voe_3d_shape_geometries *geometries,
	voe_editor_models *models, voe_math_float2 pointer, bool down,
	bool blocked);

// While holding and dragging, the ghost named by the path's last segment,
// refused or not, beside `at` in the root surface; `dim` as drag_ghost.h's.
void voe_editor_assets_drag_ghost_draw(voe_ui_context *ui,
				       const voe_ui_theme *dim,
				       const voe_editor_assets_drag *drag,
				       voe_math_float2 at);
