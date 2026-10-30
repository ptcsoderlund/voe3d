// A floating box's edge hover, press, drag and release, each frame against the
// shown boxes. The topmost box under the pointer is found in one pass over the
// closables, at most VOE_EDITOR_CLOSABLE_COUNT of them; see float_edges.h.
#include "float_edges.h"

#include <base/assert.h>

#include <math.h>

// The topmost shown floating box whose rectangle widened by the reach holds
// `at`, with `edge` set to the sides `at` is within reach of; none, and no
// edge, when `at` is on that box's body.
static voe_editor_closable edges_under(const voe_editor_dock_root *root,
				       voe_ui_rect area, voe_math_float2 at,
				       bool edge[4])
{
	const float reach = VOE_EDITOR_RESIZE_REACH;
	voe_editor_closable top = VOE_EDITOR_CLOSABLE_COUNT;
	voe_ui_rect box = { 0 };

	VOE_BASE_ASSERT(root != NULL && edge != NULL,
			"finding a box's edge on no root or into nothing");

	for (uint32_t e = 0; e < 4; e++)
		edge[e] = false;
	for (uint32_t c = 0; c < VOE_EDITOR_CLOSABLE_COUNT; c++) {
		voe_ui_rect r;

		if (!root->floats[c].floating || root->closed[c])
			continue;
		r = voe_editor_dock_float_shown(&root->floats[c], area);
		if (at.x < r.min.x - reach || at.x > r.min.x + r.size.x + reach ||
		    at.y < r.min.y - reach || at.y > r.min.y + r.size.y + reach)
			continue;
		if (top == VOE_EDITOR_CLOSABLE_COUNT ||
		    root->floats[c].raised > root->floats[top].raised) {
			top = (voe_editor_closable)c;
			box = r;
		}
	}
	if (top == VOE_EDITOR_CLOSABLE_COUNT)
		return top;

	edge[VOE_EDITOR_DOCK_EDGE_LEFT] = fabsf(at.x - box.min.x) <= reach;
	edge[VOE_EDITOR_DOCK_EDGE_RIGHT] =
		!edge[VOE_EDITOR_DOCK_EDGE_LEFT] &&
		fabsf(at.x - (box.min.x + box.size.x)) <= reach;
	edge[VOE_EDITOR_DOCK_EDGE_TOP] = fabsf(at.y - box.min.y) <= reach;
	edge[VOE_EDITOR_DOCK_EDGE_BOTTOM] =
		!edge[VOE_EDITOR_DOCK_EDGE_TOP] &&
		fabsf(at.y - (box.min.y + box.size.y)) <= reach;
	return edge[0] || edge[1] || edge[2] || edge[3] ?
		       top :
		       VOE_EDITOR_CLOSABLE_COUNT;
}

// `which`'s box drawn over every other, unless it already is.
static void raise_box(voe_editor_dock_root *root, voe_editor_closable which)
{
	uint32_t top = 0;

	VOE_BASE_ASSERT(which < VOE_EDITOR_CLOSABLE_COUNT,
			"raising a box that is not closable");

	for (uint32_t c = 0; c < VOE_EDITOR_CLOSABLE_COUNT; c++)
		if (c != (uint32_t)which && root->floats[c].raised > top)
			top = root->floats[c].raised;
	VOE_BASE_ASSERT(top < UINT32_MAX,
			"a floating box raised as high as it goes");
	if (root->floats[which].raised <= top)
		root->floats[which].raised = top + 1;
}

// The held edges moved by the pointer's travel since the press, each kept
// PANEL_MIN or FLOAT_LOW from its opposite and inside `area`, and the box
// written from the area's top left.
static void edges_drag(const voe_editor_float_edges *edges,
		       voe_editor_dock_root *root, voe_ui_rect area,
		       voe_math_float2 at)
{
	const bool *held = edges->edge;
	const float dx = at.x - edges->press.x;
	const float dy = at.y - edges->press.y;
	float left = edges->box.min.x;
	float right = left + edges->box.size.x;
	float top = edges->box.min.y;
	float bottom = top + edges->box.size.y;

	if (held[VOE_EDITOR_DOCK_EDGE_LEFT])
		left = fmaxf(fminf(left + dx, right - VOE_EDITOR_DOCK_PANEL_MIN),
			     area.min.x);
	if (held[VOE_EDITOR_DOCK_EDGE_RIGHT])
		right = fminf(fmaxf(right + dx, left + VOE_EDITOR_DOCK_PANEL_MIN),
			      area.min.x + area.size.x);
	if (held[VOE_EDITOR_DOCK_EDGE_TOP])
		top = fmaxf(fminf(top + dy, bottom - VOE_EDITOR_DOCK_FLOAT_LOW),
			    area.min.y);
	if (held[VOE_EDITOR_DOCK_EDGE_BOTTOM])
		bottom = fminf(fmaxf(bottom + dy, top + VOE_EDITOR_DOCK_FLOAT_LOW),
			       area.min.y + area.size.y);

	root->floats[edges->held].box = (voe_ui_rect){
		.min = { left - area.min.x, top - area.min.y },
		.size = { fmaxf(right - left, 0.0f), fmaxf(bottom - top, 0.0f) }
	};
	VOE_BASE_ASSERT(!isnan(left) && !isnan(top),
			"a floating box dragged to a place that is not a number");
}

voe_editor_float_edges_result
voe_editor_float_edges_frame(voe_editor_float_edges *edges,
			     voe_editor_dock_root *root, voe_ui_rect area,
			     bool allowed)
{
	voe_editor_float_edges_result result = {
		.cursor = VOE_PLATFORM_CURSOR_ARROW
	};
	voe_editor_closable which = VOE_EDITOR_CLOSABLE_COUNT;
	voe_ui_pointer pointer;
	bool edge[4] = { false, false, false, false };

	VOE_BASE_ASSERT(edges != NULL && root != NULL, "box edges with nothing");
	VOE_BASE_ASSERT(edges->held <= VOE_EDITOR_CLOSABLE_COUNT,
			"box edges holding a panel that is not closable");

	pointer = root->pointer;
	if (edges->held != VOE_EDITOR_CLOSABLE_COUNT) {
		which = edges->held;
		for (uint32_t e = 0; e < 4; e++)
			edge[e] = edges->edge[e];
		if (pointer.down) {
			edges_drag(edges, root, area, pointer.at);
		} else {
			edges->held = VOE_EDITOR_CLOSABLE_COUNT;
			result.ended = true;
		}
	} else if (allowed && pointer.over && !edges->was_down) {
		which = edges_under(root, area, pointer.at, edge);
		if (which != VOE_EDITOR_CLOSABLE_COUNT && pointer.down) {
			edges->held = which;
			for (uint32_t e = 0; e < 4; e++)
				edges->edge[e] = edge[e];
			edges->box = voe_editor_dock_float_shown(
				&root->floats[which], area);
			edges->press = pointer.at;
			raise_box(root, which);
		}
	}
	if (which != VOE_EDITOR_CLOSABLE_COUNT) {
		result.taken = true;
		result.cursor = edge[VOE_EDITOR_DOCK_EDGE_LEFT] ||
						edge[VOE_EDITOR_DOCK_EDGE_RIGHT] ?
					VOE_PLATFORM_CURSOR_LEFT_RIGHT :
					VOE_PLATFORM_CURSOR_UP_DOWN;
	}
	edges->was_down = pointer.down;
	return result;
}
