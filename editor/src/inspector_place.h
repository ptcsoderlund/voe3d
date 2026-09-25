// Where an overlay hanging in the Inspector goes this frame: the open
// dropdown's list and Add component's list under their button, and a submenu
// beside its row, and the press-outside test that closes them. Called by
// inspector_buttons.c after voe_ui_frame_end, every frame a list is open, when
// this frame's rectangles are measured (ui/layout.h).
//
//     place = voe_editor_inspector_overlay_place(&inspector, ui, button,
//                                                list, rows);
//
// A LIST GOES WHERE IT FITS, WORKED OUT EVERY FRAME. Where it sits is worked
// out from its button's rectangle and the room the panel's scroll area leaves
// round it — below the button when the whole list fits there, above it when it
// fits there instead, and on the roomier side capped to that room and
// scrolling when it fits neither (ADR-0200) — and set every frame it is open,
// because an overlay is placed where it fits each frame and never once when it
// opened (ADR-0199). That arithmetic is the button's rectangle less the
// Inspector's content column's, so it is in that column's space and says
// nothing about how far the panel is scrolled. The open dropdown's list and
// Add component's are both placed by voe_editor_inspector_overlay_place.
//
// A SUBMENU sits right of its list when its whole width fits before the right
// of the area's visible rectangle, both in the surface's millimetres, else
// left of it, and is then moved wholly inside that rectangle; its top is its
// row's, fitted by the side-and-cap rule with the row as the widget
// (ADR-0221).
#pragma once

#include "inspector.h"

#include <math/float2.h>

#include <ui/layout.h>

#include <stdbool.h>

// Whether `at` is on `rect`, the two comparisons per axis a press-outside test
// is.
bool voe_editor_inspector_rect_contains(voe_ui_rect rect, voe_math_float2 at);

// Where a list hanging from `button` goes this frame, by the side-and-cap rule
// above (ADR-0200): `list` is its panel and `rows` the area inside it as drawn
// this frame, `list` VOE_UI_NODE_NONE for one not drawn yet, which goes below
// uncapped. Needs this frame's content column.
voe_editor_inspector_place
voe_editor_inspector_overlay_place(const voe_editor_inspector *inspector,
				   const voe_ui_context *ui, voe_ui_node button,
				   voe_ui_node list, voe_ui_node rows);

// Where a submenu goes this frame (ADR-0221): beside `from`, the list it
// opened from, level with `row`; `list` is its panel and `rows` the area inside
// it, `list` VOE_UI_NODE_NONE for one not drawn yet, which goes to the right
// uncapped.
voe_editor_inspector_place
voe_editor_inspector_submenu_place(const voe_editor_inspector *inspector,
				   const voe_ui_context *ui, voe_ui_node from,
				   voe_ui_node row, voe_ui_node list,
				   voe_ui_node rows);
