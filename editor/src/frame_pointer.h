// The editor's per-frame pointer and view reads, lifted out of main.c's loop.
// main.c fills one voe_editor_frame_pointer with pointers to the parts it owns
// before the loop, beside its voe_editor_frame_commands, then calls
// voe_editor_frame_pointer_read once a frame, after the frame's input is read
// and divided into roots[0].pointer, before the views are fitted.
//
// WHAT IT OWNS: the fly's lock edge (`flew`), the borders at rest (`resize`,
// resize.h) and the left press that picks (`pick`, pick.h). The gizmo and the
// Assets drag are main.c's, because the draw reads them too.
//
// ITS ORDER, and why the order is the point:
// 1. The right button flies the view it went down over (view.h). A flying
//    view keeps every key from the shortcuts and `ui`, so it is read first.
// 2. The shortcuts, Escape's order and `ui`'s keyboard (frame_commands.h),
//    told whether a view flies; a flying view then takes `ui`'s pointer and
//    the left button.
// 3. The borders (resize.h): a seam is a fill the walk draws, not a widget,
//    so they are asked before anything else that reads the left button; one
//    they have takes `ui`'s pointer and the left button too.
// 4. The middle drag (view.h), the views' and never the interface's.
// 5. The gizmo (gizmo.h), so a press on an arrow selects nothing.
// 6. The Assets drag (assets_drag.h), blocked while the gizmo takes the press.
// 7. The pick (pick.h), last: a press the gizmo or a drag took selects nothing.
//
// Constraints: the parts pointed at are main.c's and outlive the loop; none of
// this allocates. `flew`, `resize` and `pick` are this file's to write; main.c
// sets them once in the fill and reads none of them.
#pragma once

#include "assets_drag.h"
#include "dock.h"
#include "frame_commands.h"
#include "gizmo.h"
#include "keys.h"
#include "models.h"
#include "pick.h"
#include "resize.h"
#include "scene.h"
#include "session.h"
#include "topbar.h"
#include "view.h"

#include <3d/shape_geometry.h>

#include <platform/input.h>
#include <platform/window.h>

#include <ui/widgets.h>

#include <stdbool.h>

typedef struct {
	voe_editor_frame_commands *commands;
	voe_editor_session *session;
	voe_editor_scene *scene;
	voe_editor_browser *browser;
	voe_editor_preferences *preferences;
	voe_editor_project_panel *project_panel;
	voe_editor_views *views;
	voe_editor_undo *undo;
	voe_editor_gizmo *gizmo;
	voe_editor_assets_drag *drag;
	voe_editor_dock_root *root;
	voe_editor_topbar *bar;
	const voe_3d_shape_geometries *geometries;
	voe_editor_models *models;
	// NULL in a capture: no lock, no cursor.
	voe_platform_window *window;
	// Whether a view flew last frame, so the pointer's lock is asked for
	// only on the frame that changes (platform/input.h).
	bool flew;
	// The borders between the root's panels, at rest; main.c fills it with
	// `.held = UINT32_MAX` (resize.h).
	voe_editor_resize resize;
	// The left press in a view that moves the selection (pick.h).
	voe_editor_pick pick;
} voe_editor_frame_pointer;

// One frame's input as main.c read it; zeroed in a capture.
typedef struct {
	voe_platform_pointer pointer;
	voe_platform_motion motion;
	const voe_editor_keys_frame *keyboard;
	const voe_platform_text *text;
	bool left;
	bool middle;
	bool right;
	bool shift;
	bool control;
	float pixels_per_millimetre;
	// The frame's unclamped step, which the fly moves by.
	float seconds;
} voe_editor_frame_pointer_input;

// What main.c reads after the call.
typedef struct {
	bool flying;
	// The borders had the pointer this frame.
	bool taken;
	// The keyboard `ui` is handed this frame (frame_commands.h).
	voe_ui_keyboard keyboard;
} voe_editor_frame_pointer_result;

// The seven reads above, in that order, against `root->pointer` as main.c
// divided it; writes the root's pointer clears and `lit`, not its keyboard.
voe_editor_frame_pointer_result
voe_editor_frame_pointer_read(voe_editor_frame_pointer *frame,
			      const voe_editor_frame_pointer_input *input);
