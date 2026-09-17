// The bar across the top of the root surface: New, Open and Save, then the
// project's name and whether it is unsaved, then whatever notice the session
// has to say.
//
// A FIXED HEIGHT AND NOTHING ELSE ABOUT THE LAYOUT IS THIS FILE'S. It draws one
// row, VOE_EDITOR_TOPBAR_HIGH tall, that stretches to whatever width its caller
// gives it — interface.c lays it above the dock tree and hands the tree the
// rest of the surface's height, and neither of those is a decision this file
// makes.
//
// THE THREE BUTTONS ARE RECORDED AND READ BACK, EXACTLY AS THE SCENE PANEL'S
// ROWS ARE (scene.h). A `ui` widget answers what the pointer did to it only
// after voe_ui_frame_end (ui/widgets.h), and this call returns long before
// that, so voe_editor_topbar_draw records where each button is and
// voe_editor_topbar_clicks_read asks afterwards, inside the same window
// scene.c's and inspector.c's own reads use.
//
// THE COMMAND IT HANDS BACK IS NOT CARRIED OUT HERE. This file knows nothing
// of a project, a world or an arm — it says which button fired and nothing
// more; what a command does is session.h's.
#pragma once

#include "session.h"

#include <base/arena.h>

#include <ui/layout.h>

// How tall the bar is, in the surface's own millimetres, whatever width it is
// given.
#define VOE_EDITOR_TOPBAR_HIGH 10.0f

// The bar's three buttons, recorded as they are drawn. Zeroed is a bar that has
// drawn nothing yet, which is only true before the first frame.
typedef struct {
	voe_ui_node new_button;
	voe_ui_node open_button;
	voe_ui_node save_button;
} voe_editor_topbar;

// Draws the bar as one row: New, Open, Save, then `name` with " (unsaved)"
// appended when `unsaved` is true, then `notice` when it is not empty. `arena`
// is where " (unsaved)" is composed onto `name` — the frame's own, valid for
// exactly as long as the row's labels are (ui/widgets.h). Records the three
// buttons into `bar`.
void voe_editor_topbar_draw(voe_ui_context *ui, voe_editor_topbar *bar,
			   voe_base_arena *arena, const char *name,
			   bool unsaved, const char *notice);

// Which button fired this frame, or VOE_EDITOR_COMMAND_NONE when none did.
// Called after voe_ui_frame_end and before the frame's arena is rewound — the
// one window in which a widget will answer.
voe_editor_command voe_editor_topbar_clicks_read(const voe_ui_context *ui,
						 const voe_editor_topbar *bar);
