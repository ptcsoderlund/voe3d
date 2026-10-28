// A header's press, drag, release and double-click, each frame against last
// frame's header rows (dock.h's `heads`). The floating boxes are put topmost
// first by an insertion sort of at most VOE_EDITOR_CLOSABLE_COUNT entries; the
// rest is dock.h's target, move and dock-back.
#include "panel_drag.h"

#include <base/assert.h>

#include <math.h>

static bool rect_holds(voe_ui_rect rect, voe_math_float2 p)
{
	VOE_BASE_ASSERT(rect.size.x >= 0.0f, "a rectangle that is inside out");
	VOE_BASE_ASSERT(rect.size.y >= 0.0f, "a rectangle that is inside out");

	return p.x >= rect.min.x && p.x < rect.min.x + rect.size.x &&
	       p.y >= rect.min.y && p.y < rect.min.y + rect.size.y;
}

static bool shown_floating(const voe_editor_dock_root *root,
			   voe_editor_closable which)
{
	VOE_BASE_ASSERT(root != NULL, "asking a float of no root");
	VOE_BASE_ASSERT(which < VOE_EDITOR_CLOSABLE_COUNT,
			"asking a float that is not closable");

	return root->floats[which].floating && !root->closed[which];
}

// The header under `at`, or VOE_EDITOR_CLOSABLE_COUNT: the shown floating
// boxes topmost first, a box's body hiding what is under it, then the docked
// headers; never one whose panel does not move (the bottom view).
static voe_editor_closable head_under(const voe_editor_dock_root *root,
				      voe_ui_rect area, voe_math_float2 at)
{
	voe_editor_closable order[VOE_EDITOR_CLOSABLE_COUNT];
	uint32_t count = 0;

	VOE_BASE_ASSERT(root != NULL, "finding a header on no root");
	VOE_BASE_ASSERT(area.size.x >= 0.0f && area.size.y >= 0.0f,
			"finding a header in an area that is inside out");

	for (uint32_t c = 0; c < VOE_EDITOR_CLOSABLE_COUNT; c++) {
		uint32_t i = count;

		if (!shown_floating(root, (voe_editor_closable)c))
			continue;
		count++;
		for (; i > 0 && root->floats[order[i - 1]].raised <
					root->floats[c].raised;
		     i--)
			order[i] = order[i - 1];
		order[i] = (voe_editor_closable)c;
	}
	for (uint32_t i = 0; i < count; i++) {
		if (rect_holds(root->heads[order[i]], at))
			return voe_editor_closable_panel(order[i]) !=
					       VOE_EDITOR_PANEL_COUNT ?
				       order[i] :
				       VOE_EDITOR_CLOSABLE_COUNT;
		if (rect_holds(voe_editor_dock_float_shown(
				       &root->floats[order[i]], area),
			       at))
			return VOE_EDITOR_CLOSABLE_COUNT;
	}
	for (uint32_t c = 0; c < VOE_EDITOR_CLOSABLE_COUNT; c++)
		if (!shown_floating(root, (voe_editor_closable)c) &&
		    voe_editor_closable_panel((voe_editor_closable)c) !=
			    VOE_EDITOR_PANEL_COUNT &&
		    rect_holds(root->heads[c], at))
			return (voe_editor_closable)c;
	return VOE_EDITOR_CLOSABLE_COUNT;
}

// `which`'s box drawn over every other, unless it already is.
static void raise_box(voe_editor_dock_root *root, voe_editor_closable which)
{
	uint32_t top = 0;

	VOE_BASE_ASSERT(root != NULL, "raising a box on no root");
	VOE_BASE_ASSERT(which < VOE_EDITOR_CLOSABLE_COUNT,
			"raising a box that is not closable");

	for (uint32_t c = 0; c < VOE_EDITOR_CLOSABLE_COUNT; c++)
		if (c != which && root->floats[c].raised > top)
			top = root->floats[c].raised;
	VOE_BASE_ASSERT(top < UINT32_MAX,
			"a floating box raised as high as it goes");
	if (root->floats[which].raised <= top)
		root->floats[which].raised = top + 1;
}

// A press on `which`'s header: the second within VOE_EDITOR_RESIZE_DOUBLE on
// the same floating one docks it back and ends, any other is held.
static void head_press(voe_editor_panel_drag *drag, voe_editor_dock_root *root,
		       voe_ui_rect area, voe_editor_closable which, double now,
		       voe_editor_panel_drag_result *result)
{
	const voe_math_float2 at = root->pointer.at;

	VOE_BASE_ASSERT(drag != NULL && result != NULL, "pressing into nothing");
	VOE_BASE_ASSERT(which < VOE_EDITOR_CLOSABLE_COUNT,
			"pressing a header that is not closable");

	if (drag->last == which && shown_floating(root, which) &&
	    now - drag->last_at < VOE_EDITOR_RESIZE_DOUBLE) {
		voe_editor_dock_back(root, area, which);
		drag->last = VOE_EDITOR_CLOSABLE_COUNT;
		result->ended = true;
		return;
	}
	drag->pressed = which;
	drag->press = at;
	drag->box = voe_editor_dock_panel_rect(root, area, which);
	drag->grab = (voe_math_float2){ at.x - drag->box.min.x,
					at.y - drag->box.min.y };
	drag->dragging = false;
	drag->last = which;
	drag->last_at = now;
	if (shown_floating(root, which))
		raise_box(root, which);
}

voe_editor_panel_drag_result
voe_editor_panel_drag_frame(voe_editor_panel_drag *drag,
			    voe_editor_dock_root *root, voe_ui_rect area,
			    bool allowed, double now)
{
	voe_editor_panel_drag_result result = { 0 };
	voe_ui_pointer pointer;
	voe_editor_closable which;

	VOE_BASE_ASSERT(drag != NULL && root != NULL, "a header drag with nothing");
	VOE_BASE_ASSERT(drag->pressed <= VOE_EDITOR_CLOSABLE_COUNT,
			"a header drag holding a panel that is not closable");

	pointer = root->pointer;
	if (drag->pressed != VOE_EDITOR_CLOSABLE_COUNT) {
		if (!pointer.down) {
			if (drag->dragging) {
				voe_editor_dock_move(root, area, drag->pressed,
						     &drag->target);
				result.taken = true;
				result.ended = true;
			}
			drag->pressed = VOE_EDITOR_CLOSABLE_COUNT;
			drag->dragging = false;
		} else {
			if (hypotf(pointer.at.x - drag->press.x,
				   pointer.at.y - drag->press.y) >
			    VOE_EDITOR_PANEL_DRAG_START)
				drag->dragging = true;
			if (drag->dragging) {
				drag->target = voe_editor_dock_target_at(
					root, area, drag->pressed, drag->box,
					drag->grab, pointer.at);
				result.taken = true;
			}
		}
	} else if (allowed && pointer.over && pointer.down && !drag->was_down) {
		which = head_under(root, area, pointer.at);
		if (which != VOE_EDITOR_CLOSABLE_COUNT)
			head_press(drag, root, area, which, now, &result);
	}
	drag->was_down = pointer.down;
	VOE_BASE_ASSERT(!drag->dragging || result.taken,
			"a header drag going on that did not take the pointer");
	return result;
}
