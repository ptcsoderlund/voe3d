// main.c's loop's run of pointer and view reads, moved here with its comments:
// the glide, the fly and the shortcuts, the borders, then the middle drag, the gizmo, the
// Assets drag and the pick, in the order frame_pointer.h gives. See there.
#include "frame_pointer.h"

#include "notice.h"
#include "panels.h"
#include "view_passes.h"

#include <base/assert.h>
#include <base/report.h>

#include <platform/clock.h>

// THE RIGHT BUTTON FLIES THE VIEW IT WENT DOWN OVER (view.h), never while the
// browser shows, turned by the mouse's motion and moved by W, S, A, D, E and
// Q, Shift three times as fast. While it flies the pointer is locked and
// hidden, and put back where it was on the frame it stops.
static bool fly(voe_editor_frame_pointer *frame,
		const voe_editor_frame_pointer_input *input)
{
	const bool *down = input->keyboard->down;
	bool flying;

	VOE_BASE_ASSERT(frame->views != NULL, "a fly with no views");
	flying = voe_editor_views_fly(
		frame->views, frame->root->pointer.at,
		input->right && !frame->browser->showing,
		(voe_math_float2){ input->motion.x, input->motion.y },
		(voe_editor_fly_keys){ .forward = down[VOE_PLATFORM_KEY_W],
				       .back = down[VOE_PLATFORM_KEY_S],
				       .left = down[VOE_PLATFORM_KEY_A],
				       .right = down[VOE_PLATFORM_KEY_D],
				       .up = down[VOE_PLATFORM_KEY_E],
				       .down = down[VOE_PLATFORM_KEY_Q],
				       .fast = input->shift },
		input->seconds);
	if (flying != frame->flew && frame->window != NULL)
		voe_platform_input_lock_pointer(frame->window, flying);
	frame->flew = flying;
	VOE_BASE_ASSERT(frame->flew == flying, "the fly's edge not kept");
	return flying;
}

// THE BORDERS ARE ASKED FIRST (resize.h): a seam is a fill the walk draws, not
// a widget, so no widget answers for it. While they have the pointer, `ui`
// sees no pointer and pick and the gizmo no left button; the border they
// reach is drawn lit.
static bool borders(voe_editor_frame_pointer *frame, bool flying)
{
	voe_editor_resize_result resized;

	VOE_BASE_ASSERT(frame->root != NULL && frame->bar != NULL,
			"borders with no root or bar");
	resized = voe_editor_resize_frame(
		&frame->resize, frame->root, frame->bar,
		!frame->browser->showing && !frame->preferences->showing &&
			!frame->project_panel->showing &&
			!frame->session->errors.showing &&
			!frame->scene->picking.open &&
			!frame->scene->dropdown.open &&
			!frame->bar->menu.open && !flying,
		voe_platform_clock_now());
	frame->root->lit = resized.reached;
	if (resized.taken) {
		frame->root->pointer.over = false;
		frame->root->pointer.down = false;
	}
	if (frame->window != NULL)
		voe_platform_input_cursor(frame->window, resized.cursor);
	if (resized.ended) {
		voe_base_report_error_clear();
		if (!voe_editor_panels_remember(frame->root, frame->bar))
			voe_editor_notice_from_report(&frame->session->notice,
						      "editor_settings");
	}
	VOE_BASE_ASSERT(!resized.taken || !frame->root->pointer.down,
			"the borders took a pointer `ui` still sees");
	return resized.taken;
}

// The gizmo, the Assets drag and the pick, against the left press the fly and
// the borders left.
static void presses(voe_editor_frame_pointer *frame,
		    const voe_editor_frame_pointer_input *input, bool left)
{
	const voe_math_float2 at = frame->root->pointer.at;
	const bool down = left && input->pointer.over;
	const bool panel = frame->browser->showing ||
			   frame->preferences->showing ||
			   frame->project_panel->showing ||
			   frame->session->errors.showing ||
			   frame->scene->picking.open || frame->bar->menu.open;

	VOE_BASE_ASSERT(frame->gizmo != NULL && frame->drag != NULL,
			"presses with no gizmo or drag");
	// The left half of the same division: a press on an arrow of the
	// selected entity's gizmo drags it (gizmo.h), unless a panel over the
	// views has the press instead.
	voe_editor_gizmo_read(frame->gizmo, frame->scene, frame->views,
			      VOE_EDITOR_GIZMO_MILLIMETRES *
				      input->pixels_per_millimetre,
			      at, down, panel);

	// A held model row, released over a view or the Inspector
	// (assets_drag.h), under the pick's own `blocked`.
	voe_editor_assets_drag_read(
		frame->drag, frame->session, frame->undo, frame->scene,
		frame->views, frame->root, frame->bar, frame->geometries,
		voe_editor_models_store(frame->models), at, down,
		panel || voe_editor_gizmo_taking(frame->gizmo));

	// Then a press over a view picks what is under it (pick.h). A press
	// the gizmo or a drag took is not a press that selects, and the order
	// is the point: the gizmo is asked first.
	voe_editor_pick_read(&frame->pick, frame->scene, frame->views,
			     frame->geometries,
			     voe_editor_models_store(frame->models), at, down,
			     panel || voe_editor_gizmo_taking(frame->gizmo) ||
				     frame->drag->holding);
	VOE_BASE_ASSERT(frame->scene != NULL, "presses lost the scene");
}

voe_editor_frame_pointer_result
voe_editor_frame_pointer_read(voe_editor_frame_pointer *frame,
			      const voe_editor_frame_pointer_input *input)
{
	voe_editor_frame_pointer_result result = { 0 };
	bool left;

	VOE_BASE_ASSERT(frame != NULL && input != NULL,
			"a pointer read with no frame or input");
	VOE_BASE_ASSERT(input->keyboard != NULL && input->text != NULL,
			"a pointer read with no keyboard or text");
	left = input->left;

	voe_editor_views_glide(frame->views, input->seconds);
	result.flying = fly(frame, input);

	// The shortcuts read and acted on, Escape's order, and the keyboard
	// `ui` gets (frame_commands.h); a flying view keeps the pointer from
	// `ui` too.
	result.keyboard = voe_editor_frame_commands_read(
		frame->commands, input->keyboard, input->text, left,
		result.flying);
	if (result.flying) {
		frame->root->pointer.over = false;
		frame->root->pointer.down = false;
		left = false;
	}

	result.taken = borders(frame, result.flying);
	if (result.taken)
		left = false;

	// The middle button is the views' and the left is the interface's, so
	// the two never compete for one press. Never while the browser shows —
	// "views get no drag" (browser.h) — so a press that started before it
	// opened does not carry on moving a camera underneath it.
	voe_editor_views_drag(frame->views, frame->root->pointer.at,
			      input->middle && !frame->browser->showing,
			      input->shift, input->control);

	presses(frame, input, left);
	VOE_BASE_ASSERT(!result.flying || !frame->root->pointer.down,
			"a flying view left `ui` the pointer");
	return result;
}
