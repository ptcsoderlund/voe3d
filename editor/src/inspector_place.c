// Where an overlay goes: the side-and-cap rule a list is fitted by, the list
// under its button, the submenu beside its row, and the rectangle test a press
// outside them is. See inspector_place.h for the rule and when it runs.
//
// NOTHING IN HERE WRITES. Every function reads this frame's rectangles and
// returns a place, in the Inspector's content column's space; the caller sets
// it through scene.h or on the inspector.
#include "inspector_place.h"

#include <base/assert.h>

#include <ui/layout.h>

#include <stdbool.h>

// Whether `at` is on `rect`, the two comparisons per axis a press-outside test
// is.
bool voe_editor_inspector_rect_contains(voe_ui_rect rect, voe_math_float2 at)
{
	return at.x >= rect.min.x && at.y >= rect.min.y &&
	       at.x < rect.min.x + rect.size.x &&
	       at.y < rect.min.y + rect.size.y;
}

// A list's top edge and its rows' cap, in the surface's millimetres.
typedef struct {
	float top;
	float cap;
} list_fit;

// The side-and-cap rule (ADR-0200) for the drawn `list` with its `rows` inside
// visible room `w`: its top at `down` when the whole list fits below that,
// else its bottom at `up` when it fits above, else on the roomier side capped,
// never to fewer than `least` millimetres of rows. A dropdown's `down` is its
// button's bottom and `up` its top; a submenu's are its row's top and bottom.
static list_fit side_and_cap(const voe_ui_context *ui, voe_ui_rect w,
			     voe_ui_node list, voe_ui_node rows, float down,
			     float up, float least)
{
	voe_ui_rect p = voe_ui_node_rect(ui, list);
	voe_ui_rect r = voe_ui_node_rect(ui, rows);
	// The panel's own padding and border round its rows, whether or not
	// the rows are capped.
	float chrome = p.size.y - r.size.y;
	// What the whole list would be, uncapped: what the rows wanted, which
	// voe_ui_node_measured reports even while they are capped
	// (ui/layout.h).
	float want = chrome + voe_ui_node_measured(ui, rows).y;
	float below = w.min.y + w.size.y - down;
	float above = up - w.min.y;
	list_fit fit = { .top = down, .cap = 0.0f };

	VOE_BASE_ASSERT(list != VOE_UI_NODE_NONE && rows != VOE_UI_NODE_NONE,
			"fitting a list that was not drawn");

	if (want <= below) {
		// below, the natural side
	} else if (want <= above) {
		fit.top = up - want;
	} else {
		float room = below >= above ? below : above;

		fit.cap = room - chrome;
		// A panel with almost no room either way still shows a row to
		// pick and scroll from.
		if (fit.cap < least)
			fit.cap = least;
		if (below < above)
			fit.top = up - (fit.cap + chrome);
	}
	VOE_BASE_ASSERT(fit.cap >= 0.0f, "a list capped to less than nothing");
	return fit;
}

voe_editor_inspector_place
voe_editor_inspector_overlay_place(const voe_editor_inspector *inspector,
				   const voe_ui_context *ui, voe_ui_node button,
				   voe_ui_node list, voe_ui_node rows)
{
	voe_ui_rect b;
	voe_ui_rect c;
	list_fit fit;

	VOE_BASE_ASSERT(inspector != NULL && ui != NULL,
			"placing a list on no inspector or interface");
	VOE_BASE_ASSERT(button != VOE_UI_NODE_NONE &&
				inspector->content != VOE_UI_NODE_NONE,
			"placing a list under no button or in no column");

	b = voe_ui_node_rect(ui, button);
	c = voe_ui_node_rect(ui, inspector->content);
	fit = (list_fit){ .top = b.min.y + b.size.y, .cap = 0.0f };
	if (list != VOE_UI_NODE_NONE && inspector->area != VOE_UI_NODE_NONE)
		fit = side_and_cap(ui, voe_ui_node_visible(ui, inspector->area),
				   list, rows, b.min.y + b.size.y, b.min.y,
				   b.size.y);

	return (voe_editor_inspector_place){ .left = b.min.x - c.min.x,
					     .top = fit.top - c.min.y,
					     .height = fit.cap };
}

// Where a submenu goes this frame (ADR-0221): its left edge at the right edge
// of `from`, the list it opened from, and its top at `row`'s top; its right
// edge at `from`'s left edge instead when its whole width does not fit before
// the right of the area's visible rectangle; and vertically side_and_cap with
// the row as the widget. `list` VOE_UI_NODE_NONE, not drawn yet, goes to the
// right uncapped.
//
// EITHER SIDE IS THEN MOVED WHOLLY INSIDE THAT RECTANGLE, over `from` if need
// be. A narrow Inspector has no room on either side of a list that starts at
// its left edge, and "else left" alone put the submenu wholly left of the area,
// clipped to nothing: it showed for the one frame it was not yet measured and
// then vanished, and a press on where it had been counted as outside.
voe_editor_inspector_place
voe_editor_inspector_submenu_place(const voe_editor_inspector *inspector,
				   const voe_ui_context *ui, voe_ui_node from,
				   voe_ui_node row, voe_ui_node list,
				   voe_ui_node rows)
{
	voe_ui_rect f;
	voe_ui_rect r;
	voe_ui_rect c;
	float left;
	list_fit fit;

	VOE_BASE_ASSERT(from != VOE_UI_NODE_NONE && row != VOE_UI_NODE_NONE,
			"placing a submenu beside no list or row");
	VOE_BASE_ASSERT(inspector->content != VOE_UI_NODE_NONE,
			"placing a submenu in no column");

	f = voe_ui_node_rect(ui, from);
	r = voe_ui_node_rect(ui, row);
	c = voe_ui_node_rect(ui, inspector->content);
	left = f.min.x + f.size.x;
	fit = (list_fit){ .top = r.min.y, .cap = 0.0f };
	if (list != VOE_UI_NODE_NONE && inspector->area != VOE_UI_NODE_NONE) {
		voe_ui_rect w = voe_ui_node_visible(ui, inspector->area);
		float width = voe_ui_node_rect(ui, list).size.x;

		if (left + width > w.min.x + w.size.x)
			left = f.min.x - width;
		if (left + width > w.min.x + w.size.x)
			left = w.min.x + w.size.x - width;
		if (left < w.min.x)
			left = w.min.x;
		fit = side_and_cap(ui, w, list, rows, r.min.y,
				   r.min.y + r.size.y, r.size.y);
	}

	return (voe_editor_inspector_place){ .left = left - c.min.x,
					     .top = fit.top - c.min.y,
					     .height = fit.cap };
}
