// A panel's header dragged to dock or float it, and a double-click on a
// floating one docking it back (0439, 0491 point 4). frame_pointer.c asks
// voe_editor_panel_drag_frame once a frame, before the borders (resize.h),
// with the dock area below the bar and the same `allowed` they get.
//
//     voe_editor_panel_drag moving = { .pressed = VOE_EDITOR_CLOSABLE_COUNT,
//                                      .last = VOE_EDITOR_CLOSABLE_COUNT };
//
// WHAT IT OWNS: the press on a header, where the panel was and the grab into
// it then, whether a drag is going on and the target it would drop on, and the
// last press for the double-click. The tree and the floating boxes are the
// root's, written here only through dock.h's move and dock-back, and a press's
// raise of a floating box.
//
// THE HEADERS ARE LAST FRAME'S (dock.h's `heads`), as `ui` laid them out: the
// press edge is read before this frame's walk. Floating shown boxes are asked
// topmost first, a press on one's body hitting nothing beneath it (0491 point
// 6), then the docked headers; the bottom view's header never drags (0491
// point 1).
//
// BEFORE THE BORDERS, AND TAKING NOTHING UNTIL IT MOVES. A press on a header
// is remembered but not taken, so the × still clicks; only once the pointer
// has gone past VOE_EDITOR_PANEL_DRAG_START from the press (0282) is it a drag,
// taken every frame until the release, which moves the panel to the target
// voe_editor_dock_target_at answered last (dock.h). A panel dropped where it
// is docks or floats there again; nothing refuses a drop.
//
// THE DOUBLE-CLICK IS TIMED HERE, as resize.h times a border's: a second press
// on the same floating header within VOE_EDITOR_RESIZE_DOUBLE on the caller's
// clock is voe_editor_dock_back, and ends.
//
// Constraints: nothing here writes a file; the caller remembers the layout
// when a drag or double-click ends. A zeroed struct names closable 0 as
// pressed, so it is filled as above.
#pragma once

#include "dock.h"
#include "resize.h"

#include <stdbool.h>

// Millimetres a held header must move from its press before it drags.
#define VOE_EDITOR_PANEL_DRAG_START 1.0f

// `pressed` is the header held, VOE_EDITOR_CLOSABLE_COUNT for none; `press`
// the pointer at the press, `box` the panel's rectangle then and `grab` the
// pointer's offset into it; `dragging` once past the start, with `target`
// where a release would drop it. `last` and `last_at` are the last press's
// closable and time, `was_down` last frame's button level.
typedef struct {
	voe_editor_closable pressed;
	voe_math_float2 press;
	voe_ui_rect box;
	voe_math_float2 grab;
	bool dragging;
	voe_editor_dock_target target;
	voe_editor_closable last;
	double last_at;
	bool was_down;
} voe_editor_panel_drag;

// Whether the drag had the pointer this frame, and whether a drop or a
// double-click ended, so the layout is to be remembered.
typedef struct {
	bool taken;
	bool ended;
} voe_editor_panel_drag_result;

// One frame against `root`'s pointer, `area` the dock area in the root's
// millimetres. `allowed` false while a panel covers the dock or a view flies:
// no press starts. `now` is seconds on a monotonic clock.
voe_editor_panel_drag_result
voe_editor_panel_drag_frame(voe_editor_panel_drag *drag,
			    voe_editor_dock_root *root, voe_ui_rect area,
			    bool allowed, double now);
