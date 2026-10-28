// main.c's loop's run of pointer and view reads, moved here with its comments:
// the glide, the fly and the shortcuts, a floating box's edges, a header's
// drag and the borders, the Assets panel's right button, then the middle drag,
// the brush, the gizmo, the Assets drag and the pick, in the order
// frame_pointer.h gives, each kept from starting under a floating box. See
// there.
#include "frame_pointer.h"

#include "frame_selection.h"
#include "notice.h"
#include "panels.h"
#include "view_passes.h"

#include <base/assert.h>
#include <base/report.h>

#include <platform/clock.h>

// Where the views are told the pointer is while a floating box is over it: left
// of and above every view, so a button's down edge there finds none and is
// still the edge the views remember (view.h).
static const voe_math_float2 NO_VIEW = { -1.0f, -1.0f };

// The dock area below the bar, in the root's millimetres.
static voe_ui_rect dock_area(const voe_editor_frame_pointer *frame)
{
	const float high =
		voe_editor_topbar_high(frame->bar, frame->root->size.y);

	return (voe_ui_rect){ .min = { 0.0f, high },
			      .size = { frame->root->size.x,
					frame->root->size.y - high } };
}

// THE RIGHT BUTTON FLIES THE VIEW IT WENT DOWN OVER (view.h), never while the
// browser shows, turned by the mouse's motion and moved by W, S, A, D, E and
// Q, Shift three times as fast, the wheel setting its speed. While it flies
// the pointer is locked and hidden, and put back where it was on the frame it
// stops. The wheel's +y is towards the person, so a notch up is −y. A fly
// does not start under a floating box (`floated`).
static bool fly(voe_editor_frame_pointer *frame,
		const voe_editor_frame_pointer_input *input, bool floated)
{
	const bool *down = input->keyboard->down;
	bool flying;

	VOE_BASE_ASSERT(frame->views != NULL, "a fly with no views");
	flying = voe_editor_views_fly(
		frame->views,
		floated && !frame->flew ? NO_VIEW : frame->root->pointer.at,
		input->right && !frame->browser->showing,
		(voe_math_float2){ input->motion.x, input->motion.y },
		(voe_editor_fly_keys){ .forward = down[VOE_PLATFORM_KEY_W],
				       .back = down[VOE_PLATFORM_KEY_S],
				       .left = down[VOE_PLATFORM_KEY_A],
				       .right = down[VOE_PLATFORM_KEY_D],
				       .up = down[VOE_PLATFORM_KEY_E],
				       .down = down[VOE_PLATFORM_KEY_Q],
				       .fast = input->shift,
				       .notches = -input->wheel.y,
				       .middle = input->middle },
		input->seconds);
	if (flying != frame->flew && frame->window != NULL)
		voe_platform_input_lock_pointer(frame->window, flying);
	frame->flew = flying;
	VOE_BASE_ASSERT(frame->flew == flying, "the fly's edge not kept");
	return flying;
}

// The layout remembered when a drag or double-click ends, a failure said in
// the notice (panels.h).
static void layout_remember(voe_editor_frame_pointer *frame)
{
	VOE_BASE_ASSERT(frame->root != NULL && frame->bar != NULL,
			"remembering the layout of no root or bar");
	VOE_BASE_ASSERT(frame->session != NULL,
			"remembering the layout with no notice to say a failure in");
	voe_base_report_error_clear();
	if (!voe_editor_panels_remember(frame->root, frame->bar))
		voe_editor_notice_from_report(&frame->session->notice,
					      "editor_settings");
}

// THE BORDERS (resize.h): a seam is a fill the walk draws, not a widget, so no
// widget answers for it. While they have the pointer, `ui` sees no pointer and
// pick and the gizmo no left button; the border they reach is drawn lit.
static bool borders(voe_editor_frame_pointer *frame, bool allowed, double now)
{
	voe_editor_resize_result resized;

	VOE_BASE_ASSERT(frame->root != NULL && frame->bar != NULL,
			"borders with no root or bar");
	resized = voe_editor_resize_frame(&frame->resize, frame->root,
					  frame->bar, allowed, now);
	frame->root->lit = resized.reached;
	if (resized.taken) {
		frame->root->pointer.over = false;
		frame->root->pointer.down = false;
	}
	if (frame->window != NULL)
		voe_platform_input_cursor(frame->window, resized.cursor);
	if (resized.ended)
		layout_remember(frame);
	VOE_BASE_ASSERT(!resized.taken || !frame->root->pointer.down,
			"the borders took a pointer `ui` still sees");
	return resized.taken;
}

// A FLOATING BOX'S EDGES (float_edges.h), taken as the borders' are: while
// they have the pointer `ui` sees none, no seam is lit and their cursor is
// shown. An end remembers the layout.
static bool box_edges(voe_editor_frame_pointer *frame, voe_ui_rect area,
		      bool allowed)
{
	voe_editor_float_edges_result edged;

	edged = voe_editor_float_edges_frame(&frame->edges, frame->root, area,
					     allowed);
	if (edged.ended)
		layout_remember(frame);
	if (!edged.taken)
		return false;
	frame->root->lit = UINT32_MAX;
	frame->root->pointer.over = false;
	frame->root->pointer.down = false;
	if (frame->window != NULL)
		voe_platform_input_cursor(frame->window, edged.cursor);
	VOE_BASE_ASSERT(!frame->root->pointer.down,
			"the box edges took a pointer `ui` still sees");
	return true;
}

// A BOX'S EDGES, THEN A HEADER'S DRAG (panel_drag.h), THEN THE BORDERS, in the
// dock area below the bar and under one `allowed`. Edges that have the pointer
// keep the other two from being asked; while a header drags, `ui` sees no
// pointer, no seam is lit and the borders are not asked; a press the borders
// then hold is theirs, not a header's beside the seam. No seam starts under a
// floating box (`floated`).
static bool headers_and_borders(voe_editor_frame_pointer *frame, bool flying,
				bool floated)
{
	const bool allowed = !frame->browser->showing &&
			     !frame->preferences->showing &&
			     !frame->scene->picking.open &&
			     !frame->scene->dropdown.open &&
			     !frame->bar->menu.open &&
			     !frame->scene->assets.menu.open && !flying;
	const double now = voe_platform_clock_now();
	const voe_ui_rect area = dock_area(frame);
	voe_editor_panel_drag_result moved;
	bool taken;

	VOE_BASE_ASSERT(frame->root != NULL && frame->bar != NULL,
			"a header drag with no root or bar");
	if (box_edges(frame, area, allowed))
		return true;
	moved = voe_editor_panel_drag_frame(&frame->moving, frame->root, area,
					    allowed, now);
	// What the walk outlines and the ghost names (dock.h).
	frame->root->moving = frame->moving.dragging ?
				      frame->moving.pressed :
				      VOE_EDITOR_CLOSABLE_COUNT;
	frame->root->outline = frame->moving.target.outline;
	if (moved.ended)
		layout_remember(frame);
	if (moved.taken) {
		frame->root->lit = UINT32_MAX;
		frame->root->pointer.over = false;
		frame->root->pointer.down = false;
		return true;
	}
	taken = borders(frame, allowed && !floated, now);
	if (frame->resize.held != UINT32_MAX)
		frame->moving.pressed = VOE_EDITOR_CLOSABLE_COUNT;
	VOE_BASE_ASSERT(!taken || !frame->root->pointer.down,
			"the borders took a pointer `ui` still sees");
	return taken;
}

// THE RIGHT BUTTON'S DOWN EDGE OVER THE ASSETS PANEL OPENS ITS MENU (0378
// point 8): it selects the row under the pointer, gives the panel the
// keyboard and opens the menu there (assets_panel.h), by last frame's
// rectangles, since `ui` knows one button. Never while a view flies or a
// panel covers the dock.
static void assets_right(voe_editor_frame_pointer *frame,
			 const voe_editor_frame_pointer_input *input, bool flying)
{
	const bool edge = input->right && !frame->right_was;
	const bool covered = frame->browser->showing ||
			     frame->preferences->showing ||
			     frame->session->asking.open || frame->bar->menu.open;

	frame->right_was = input->right;
	if (!edge || flying || covered ||
	    !voe_editor_dock_over_panel(
		    frame->root,
		    voe_editor_topbar_high(frame->bar, frame->root->size.y),
		    VOE_EDITOR_PANEL_ASSETS, frame->root->pointer.at))
		return;
	(void)voe_editor_assets_menu_at(&frame->scene->assets,
					frame->root->pointer.at);
	VOE_BASE_ASSERT(frame->right_was, "a right edge not kept");
}

// The brush, the gizmo, the Assets drag and the pick, against the left press
// the fly and the borders left. `under` is the floating box at the pointer,
// VOE_EDITOR_CLOSABLE_COUNT for none: no stroke or gizmo drag starts under
// one and nothing is picked, and the Assets drag is blocked under any but the
// Assets panel's and the Inspector's, its source and its target.
static void presses(voe_editor_frame_pointer *frame,
		    const voe_editor_frame_pointer_input *input, bool left,
		    voe_editor_closable under)
{
	const voe_math_float2 at = frame->root->pointer.at;
	const bool down = left && input->pointer.over;
	const bool floated = under != VOE_EDITOR_CLOSABLE_COUNT;
	const bool foreign = floated && under != VOE_EDITOR_CLOSABLE_ASSETS &&
			     under != VOE_EDITOR_CLOSABLE_INSPECTOR;
	bool panel = frame->browser->showing || frame->preferences->showing ||
		     frame->scene->picking.open || frame->bar->menu.open ||
		     frame->scene->assets.menu.open;

	VOE_BASE_ASSERT(frame->gizmo != NULL && frame->drag != NULL,
			"presses with no gizmo or drag");
	// A press on the selection's ground, with a brush chosen, sculpts it
	// (sculpt.h); while it takes the press or a stroke is held, the rest
	// below are blocked as under a panel.
	if (voe_editor_sculpt_read(&frame->scene->sculpt, frame->scene,
				   frame->views, frame->models, frame->undo,
				   frame->session, at, down, !panel && !floated,
				   input->seconds, frame->scratch) ||
	    frame->scene->sculpt.stroking)
		panel = true;
	// The left half of the same division: a press on an arrow of the
	// selected entity's gizmo drags it (gizmo.h), unless a panel over the
	// views has the press instead. Ctrl held flips snap for that frame.
	voe_editor_gizmo_read(frame->gizmo, frame->scene, frame->views,
			      VOE_EDITOR_GIZMO_MILLIMETRES *
				      input->pixels_per_millimetre,
			      at, down,
			      panel || (floated &&
					!voe_editor_gizmo_taking(frame->gizmo)),
			      input->control);

	// A held Assets row, released over a view, the Inspector, a folder
	// row or Up (assets_drag.h), under the pick's own `blocked`.
	voe_editor_assets_drag_read(
		frame->drag, frame->session, frame->undo, frame->scene,
		frame->views, frame->root, frame->bar, frame->geometries,
		frame->models, at, down,
		panel || voe_editor_gizmo_taking(frame->gizmo) || foreign);

	// Then a press over a view picks what is under it (pick.h). A press
	// the gizmo or a drag took is not a press that selects, and the order
	// is the point: the gizmo is asked first.
	voe_editor_pick_read(&frame->pick, frame->scene, frame->views,
			     frame->geometries,
			     voe_editor_models_store(frame->models), at, down,
			     panel || voe_editor_gizmo_taking(frame->gizmo) ||
				     frame->drag->holding || floated);
	VOE_BASE_ASSERT(frame->scene != NULL, "presses lost the scene");
}

voe_editor_frame_pointer_result
voe_editor_frame_pointer_read(voe_editor_frame_pointer *frame,
			      const voe_editor_frame_pointer_input *input)
{
	voe_editor_frame_pointer_result result = { 0 };
	voe_editor_closable under;
	bool floated;
	bool left;

	VOE_BASE_ASSERT(frame != NULL && input != NULL,
			"a pointer read with no frame or input");
	VOE_BASE_ASSERT(input->keyboard != NULL && input->text != NULL,
			"a pointer read with no keyboard or text");
	left = input->left;
	// The floating box at the pointer, whose panel the pointer is (0491
	// point 6): nothing below starts under it.
	under = voe_editor_dock_float_under(frame->root, dock_area(frame),
					    frame->root->pointer.at);
	floated = under != VOE_EDITOR_CLOSABLE_COUNT;

	voe_editor_views_glide(frame->views, input->seconds);
	result.flying = fly(frame, input, floated);

	// The shortcuts read and acted on, Escape's order, and the keyboard
	// `ui` gets (frame_commands.h); a flying view keeps the pointer from
	// `ui` too.
	result.keyboard = voe_editor_frame_commands_read(
		frame->commands, input->keyboard, input->text, left,
		result.flying);
	// F: the view under the pointer glides to the selection
	// (frame_selection.h), against what the pick is handed.
	if (frame->commands->shortcuts.frame_selection)
		voe_editor_frame_selection(
			frame->views, frame->scene, frame->geometries,
			voe_editor_models_store(frame->models),
			frame->root->pointer.at);
	// A flying view took the wheel too, so `ui` scrolls nothing under it.
	if (result.flying) {
		frame->root->pointer.over = false;
		frame->root->pointer.down = false;
		frame->root->pointer.scroll = (voe_math_float2){ 0 };
		left = false;
	}

	result.taken = headers_and_borders(frame, result.flying, floated);
	if (result.taken)
		left = false;
	assets_right(frame, input, result.flying);

	// The middle button is the views' and the left is the interface's, so
	// the two never compete for one press. Never while the browser shows —
	// "views get no drag" (browser.h) — so a press that started before it
	// opened does not carry on moving a camera underneath it. None starts
	// under a floating box; one going on carries on over it.
	voe_editor_views_drag(
		frame->views,
		floated && frame->views->captured == VOE_EDITOR_VIEW_NONE ?
			NO_VIEW :
			frame->root->pointer.at,
		input->middle && !frame->browser->showing, input->shift,
		input->control);

	presses(frame, input, left, under);
	VOE_BASE_ASSERT(!result.flying || !frame->root->pointer.down,
			"a flying view left `ui` the pointer");
	return result;
}
