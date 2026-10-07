// The menu the right button opens over the Assets panel (0377 point 2, 0378
// point 8). Over a row it holds Rename, Duplicate and Delete; over the empty
// part under the rows it holds Create, which opens a submenu beside it holding
// the kinds a person can make there. Folder is the one kind today; LATER KINDS
// ADD THEIR ROW TO CREATE'S SUBMENU in assets_menu.c's kind tables and an item
// here (Material in 084, Landscape in 082).
//
//     voe_editor_assets_menu_open(&menu, at, on_row);   // assets_panel.c
//     if (menu.open)
//             voe_editor_assets_menu_draw(ui, &menu);
//     ...     // after voe_ui_frame_end
//     item = voe_editor_assets_menu_read(ui, &menu, at, down, surface);
//
// IT IS DRAWN AND READ THE WAY panels_menu.h's list is: an anchored raised
// panel that takes the pointer, every row as wide as the widest row's content
// as the read measured it, natural on the first frame. It hangs at the pointer
// where it was opened, moved left or above it when it would leave the surface.
// Create's submenu goes beside it by inspector_place.h's rule (ADR-0221):
// right of the menu when its whole width fits on the surface, else left, then
// moved wholly onto the surface, its top level with Create's row unless that
// would leave the surface below. Both are worked out by the read from the
// rectangles drawn this frame and used by the next draw, so a menu near an
// edge shows one frame where it would not fit.
//
// IT CLOSES ITSELF on a fired row, or a primary press on neither the menu nor
// its submenu; Create toggles the submenu and closes nothing. Escape, and the
// overlays that cover the dock, are the caller's to close it for (interface.c).
// Carrying out a fired row is the caller's too: this file only draws and reads.
//
// Constraints: drawing or reading a menu that is not open asserts. The surface
// is the root's, in millimetres from its top-left corner, as the pointer is.
#pragma once

#include <math/float2.h>

#include <ui/layout.h>

#include <stdbool.h>
#include <stdint.h>

// A row's menu: Rename, Duplicate and Delete.
#define VOE_EDITOR_ASSETS_MENU_ROWS 3
// Create's submenu: Folder.
#define VOE_EDITOR_ASSETS_MENU_KINDS 1

// What a read found fired.
typedef enum {
	VOE_EDITOR_ASSETS_MENU_NONE = 0,
	VOE_EDITOR_ASSETS_MENU_RENAME,
	VOE_EDITOR_ASSETS_MENU_DUPLICATE,
	VOE_EDITOR_ASSETS_MENU_DELETE,
	VOE_EDITOR_ASSETS_MENU_FOLDER,
} voe_editor_assets_menu_item;

// Whether the menu shows, whether it was opened over a row (else over the
// empty part, holding Create), the pointer it was opened at, and whether
// Create's submenu shows. `place` and `kinds_place` are where the next draw
// puts the menu and the submenu; the nodes are this frame's, VOE_UI_NODE_NONE
// for one not drawn; the widths are the widest row's content last frame,
// nought until measured. Zeroed is closed.
typedef struct {
	bool open;
	bool on_row;
	voe_math_float2 at;
	bool create_open;
	voe_math_float2 place;
	voe_math_float2 kinds_place;
	voe_ui_node panel;
	voe_ui_node rows[VOE_EDITOR_ASSETS_MENU_ROWS];
	voe_ui_node names[VOE_EDITOR_ASSETS_MENU_ROWS];
	float rows_wide;
	voe_ui_node kinds_panel;
	voe_ui_node kinds[VOE_EDITOR_ASSETS_MENU_KINDS];
	voe_ui_node kind_names[VOE_EDITOR_ASSETS_MENU_KINDS];
	float kinds_wide;
} voe_editor_assets_menu;

// Opens the menu at `at`, a row's menu when `on_row`, else Create, its
// submenu shut and its widths measured afresh.
void voe_editor_assets_menu_open(voe_editor_assets_menu *menu,
				 voe_math_float2 at, bool on_row);

// Closes the menu and its submenu.
void voe_editor_assets_menu_close(voe_editor_assets_menu *menu);

// Draws the menu, and Create's submenu while it shows, recording their nodes.
void voe_editor_assets_menu_draw(voe_ui_context *ui, voe_editor_assets_menu *menu);

// After voe_ui_frame_end, in the frame the menu was drawn: the row that fired,
// NONE for none; Create toggles its submenu; a fired row, or `down` (the
// primary button held) with `at` on neither panel, closes the menu. Measures
// the widths and places both for the next draw within `surface`.
voe_editor_assets_menu_item
voe_editor_assets_menu_read(const voe_ui_context *ui,
			    voe_editor_assets_menu *menu, voe_math_float2 at,
			    bool down, voe_math_float2 surface);
