// The list the top bar's Panels button opens (0351, 0363 point 3): one row per
// closable panel in voe_editor_closable's order, Scene list, Assets, Inspector,
// Bottom view, Project, Errors, an open one ticked.
//
//     if (menu.open)
//             voe_editor_panels_menu_draw(ui, &menu, under, open);
//     ...     // after voe_ui_frame_end
//     fired = voe_editor_panels_menu_read(ui, &menu, at, &over);
//
// THE TICK IS √ (U+221A), A LABEL, because Oxanium carries no ✓ and a label
// costs nothing new (0363); a choice's selected state would read as "chosen",
// not "open". It sits in a column of its own, as wide with or without it, so
// the names line up.
//
// IT IS AN ANCHORED PANEL HANGING BELOW `under`, the Panels button's rectangle
// last frame, in the root column's millimetres, and it takes the pointer, so
// nothing under it hovers or takes a click.
//
// HOW IT CLOSES IS THE CALLER'S (interface.c): a fired row toggles its panel
// and closes the list; Escape, a press outside the list and the button, or the
// browser showing close it too. This file only draws and reads.
//
// Constraints: drawing a list that is not open asserts. The tick column is a
// fixed width in millimetres, wide enough for √ at the text sizes Preferences
// offers; a larger text size would need it measured instead.
#pragma once

#include "dock.h"

#include <math/float2.h>

#include <ui/layout.h>

#include <stdbool.h>

// Whether the list shows, and its panel and rows as drawn this frame, for the
// read after voe_ui_frame_end; VOE_UI_NODE_NONE for a node not drawn. Zeroed
// is closed: a closed list is never read.
typedef struct {
	bool open;
	voe_ui_node panel;
	voe_ui_node rows[VOE_EDITOR_CLOSABLE_COUNT];
} voe_editor_panels_menu;

// Draws the list below `under`, each row ticked when `open` says so, and
// records its panel and rows into `menu`. Asserts when `menu` is not open.
void voe_editor_panels_menu_draw(voe_ui_context *ui, voe_editor_panels_menu *menu,
				 voe_ui_rect under,
				 const bool open[VOE_EDITOR_CLOSABLE_COUNT]);

// The row that fired this frame, VOE_EDITOR_CLOSABLE_COUNT for none, and in
// `over` whether `at` is on the list's visible rectangle, for the press
// outside. Called after voe_ui_frame_end, in the frame the list was drawn.
voe_editor_closable voe_editor_panels_menu_read(const voe_ui_context *ui,
						const voe_editor_panels_menu *menu,
						voe_math_float2 at, bool *over);
