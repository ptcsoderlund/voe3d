// A floating panel's edges dragged to resize its box (0439, 0491 point 3).
// frame_pointer.c asks voe_editor_float_edges_frame once a frame, first in its
// step 3: before a header's drag (panel_drag.h) and the seams (resize.h), with
// the dock area below the bar and the same `allowed` they get.
//
//     voe_editor_float_edges edges = { .held = VOE_EDITOR_CLOSABLE_COUNT };
//
// WHAT IT OWNS: the closable whose edges are held, which of the four, the box
// and the pointer at the press, and last frame's button. The box itself is the
// root's `floats`, written here only as a drag's new `box` and a press's raise.
//
// NOT THE SEAMS' FILE, BECAUSE A BOX IS NOT A SPLIT. A seam is a node of the
// tree, written through voe_editor_dock_split_set against the arrangement and
// bounded by what its two sides need; a floating box is four numbers on the
// root beside the tree, bounded by 0491 point 3's 30 × 20 mm and the dock
// area. What the two share is the band: VOE_EDITOR_RESIZE_REACH either side
// (0226), so a box's edge is as easy to take as a seam.
//
// AN EDGE IS A BAND ABOUT A SHOWN BOX'S SIDE, TWO AT A CORNER. The boxes are
// asked topmost first: the topmost whose rectangle widened by the reach holds
// the pointer is the one asked, so a box's body hides the edges of the boxes
// beneath it. Hover and press need the button up the frame before, as a seam's
// do. The drag moves the held edges by the pointer's travel since the press,
// the box kept at least PANEL_MIN × FLOAT_LOW and inside the area and written
// from the area's top left; the press raises the box; the release ends.
//
// Constraints: nothing here writes a file; the caller remembers the layout
// when a drag ends. A zeroed struct names closable 0 as held, so it is filled
// as above. At a corner the cursor is LEFT_RIGHT: `platform` has no diagonal
// shape, and one there would lift that.
#pragma once

#include "dock.h"
#include "resize.h"

#include <platform/input.h>

#include <stdbool.h>

// `held` is the closable whose edges are dragged, VOE_EDITOR_CLOSABLE_COUNT
// for none; `edge` which of them, indexed by voe_editor_dock_edge; `box` its
// shown box at the press in the root's millimetres and `press` the pointer
// then; `was_down` last frame's button level.
typedef struct {
	voe_editor_closable held;
	bool edge[4];
	voe_ui_rect box;
	voe_math_float2 press;
	bool was_down;
} voe_editor_float_edges;

// The pointer's shape this frame, whether the edges took the pointer, and
// whether a drag ended, so the layout is to be remembered.
typedef struct {
	voe_platform_cursor cursor;
	bool taken;
	bool ended;
} voe_editor_float_edges_result;

// One frame against `root`'s pointer, `area` the dock area in the root's
// millimetres. `allowed` false while a panel covers the dock or a view flies:
// nothing is hovered and no drag starts. Cursor LEFT_RIGHT on a side edge,
// UP_DOWN on a top or bottom one.
voe_editor_float_edges_result
voe_editor_float_edges_frame(voe_editor_float_edges *edges,
			     voe_editor_dock_root *root, voe_ui_rect area,
			     bool allowed);
