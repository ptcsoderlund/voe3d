// The Project panel's one frame of `ui` calls and the read of its controls
// afterwards. See the header for why a change is handed back rather than
// carried out, and why it is not undone.
#include "project_panel.h"

#include <base/assert.h>

#include <stdio.h>

// The same plate, padding and gaps preferences.c's panel uses, so the
// floating panels read as one kind of thing. Millimetres.
#define PROJECT_PANEL_PAD 3.0f
#define PROJECT_PANEL_GAP 2.0f

// What one millimetre of drag across a size box is worth, in pixels.
#define PROJECT_PANEL_STEP 1.0

void voe_editor_project_panel_show(voe_editor_project_panel *panel)
{
	VOE_BASE_ASSERT(panel != NULL, "showing no project panel");

	panel->showing = true;
}

void voe_editor_project_panel_hide(voe_editor_project_panel *panel)
{
	VOE_BASE_ASSERT(panel != NULL, "hiding no project panel");

	panel->showing = false;
}

// One row of a name and its number box, the box's text formatted into `text`.
static voe_ui_node size_row(voe_ui_context *ui, const char *name, int value,
			    char *text, size_t text_size)
{
	voe_ui_node box;

	VOE_BASE_ASSERT(name != NULL && text != NULL, "a size row with no text");

	voe_ui_row_begin(ui, (voe_ui_container){
				     .across = VOE_UI_ACROSS_CENTER,
				     .gap = PROJECT_PANEL_GAP });
	voe_ui_label(ui, name);
	box = voe_ui_number_begin(ui, name, 0, (double)value,
				  PROJECT_PANEL_STEP);
	(void)snprintf(text, text_size, "%d", value);
	voe_ui_label(ui, text);
	voe_ui_end(ui); // number box
	voe_ui_end(ui); // row

	return box;
}

void voe_editor_project_panel_draw(voe_ui_context *ui,
				   voe_editor_project_panel *panel,
				   voe_authoring_project_window window,
				   float top, voe_math_float2 size)
{
	VOE_BASE_ASSERT(ui != NULL, "drawing no project panel into no interface");
	VOE_BASE_ASSERT(panel != NULL, "drawing no project panel");
	VOE_BASE_ASSERT(panel->showing,
			"drawing a project panel that is not showing");

	panel->shown = window;

	// Anchored exactly as preferences.c's panel is.
	voe_ui_panel_begin(
		ui, "project", 0, VOE_UI_SURFACE_RAISED,
		(voe_ui_container){
			.across = VOE_UI_ACROSS_FILL,
			.gap = PROJECT_PANEL_GAP,
			.pad = { PROJECT_PANEL_PAD, PROJECT_PANEL_PAD,
				 PROJECT_PANEL_PAD, PROJECT_PANEL_PAD },
			.anchor = { .anchored = true,
				    .x = { VOE_UI_ACROSS_FILL, 0.0f },
				    .y = { VOE_UI_ACROSS_START, top } },
			.size = { .across = { VOE_UI_SIZE_FIXED, size.y } } });

	panel->width_box = size_row(ui, "Width", window.width,
				    panel->width_text, sizeof panel->width_text);
	panel->height_box = size_row(ui, "Height", window.height,
				     panel->height_text,
				     sizeof panel->height_text);

	voe_ui_row_begin(ui, (voe_ui_container){
				     .across = VOE_UI_ACROSS_CENTER,
				     .gap = PROJECT_PANEL_GAP });
	panel->windowed_choice =
		voe_ui_choice_begin(ui, "windowed", 0, !window.fullscreen);
	voe_ui_label(ui, "Windowed");
	voe_ui_end(ui); // windowed choice
	panel->fullscreen_choice =
		voe_ui_choice_begin(ui, "fullscreen", 0, window.fullscreen);
	voe_ui_label(ui, "Fullscreen");
	voe_ui_end(ui); // fullscreen choice
	voe_ui_end(ui); // choice row

	// In a row of its own so it keeps its natural width, as
	// preferences.c's Close does.
	voe_ui_row_begin(ui, (voe_ui_container){ .across = VOE_UI_ACROSS_CENTER,
						 .gap = PROJECT_PANEL_GAP });
	panel->close_button = voe_ui_button_begin(ui, "close", 0);
	voe_ui_label(ui, "Close");
	voe_ui_end(ui); // close button
	voe_ui_end(ui); // bottom row

	voe_ui_end(ui); // panel
}

// A box's value rounded to a whole number inside the project file's range.
static int size_clamped(double value)
{
	int out;

	if (value < VOE_AUTHORING_PROJECT_WINDOW_MIN)
		value = VOE_AUTHORING_PROJECT_WINDOW_MIN;
	if (value > VOE_AUTHORING_PROJECT_WINDOW_MAX)
		value = VOE_AUTHORING_PROJECT_WINDOW_MAX;
	out = (int)(value + 0.5);
	VOE_BASE_ASSERT(out >= VOE_AUTHORING_PROJECT_WINDOW_MIN &&
				out <= VOE_AUTHORING_PROJECT_WINDOW_MAX,
			"a clamped size out of range");

	return out;
}

// Whether `box` was dragged or committed this frame, into `size` when so.
static void size_read(const voe_ui_context *ui, voe_ui_node box, int *size)
{
	voe_ui_number_result number;

	if (box == VOE_UI_NODE_NONE)
		return;
	number = voe_ui_number_action(ui, box);
	if (number.changed)
		*size = size_clamped(number.value);
}

// Whether `node`, drawn or not, fired this frame.
static bool fired(const voe_ui_context *ui, voe_ui_node node)
{
	return node != VOE_UI_NODE_NONE && voe_ui_button_action(ui, node).fired;
}

voe_editor_project_panel_result
voe_editor_project_panel_clicks_read(const voe_ui_context *ui,
				     const voe_editor_project_panel *panel)
{
	VOE_BASE_ASSERT(ui != NULL, "reading the clicks of no interface");
	VOE_BASE_ASSERT(panel != NULL, "reading the clicks of no project panel");

	// A refused frame hands back VOE_UI_NODE_NONE past the node budget,
	// and those are skipped, exactly as preferences.c's are.
	voe_editor_project_panel_result result = { .window = panel->shown };

	size_read(ui, panel->width_box, &result.window.width);
	size_read(ui, panel->height_box, &result.window.height);
	if (fired(ui, panel->windowed_choice))
		result.window.fullscreen = false;
	if (fired(ui, panel->fullscreen_choice))
		result.window.fullscreen = true;
	result.closed = fired(ui, panel->close_button);
	result.changed = result.window.width != panel->shown.width ||
			 result.window.height != panel->shown.height ||
			 result.window.fullscreen != panel->shown.fullscreen;

	return result;
}
