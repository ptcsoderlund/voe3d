// The Landscape panel's one frame of `ui` calls and the read of its controls
// afterwards. See the header for why a change is handed back rather than
// carried out, and why it is not undone.
#include "landscape_panel.h"

#include "themes.h"

#include <assets/landscape.h>

#include <base/assert.h>

#include <platform/path.h>

#include <stdio.h>
#include <string.h>

// The same plate, padding and gaps project_panel.c's panel uses. Millimetres.
#define LANDSCAPE_PANEL_PAD (3.0f * VOE_EDITOR_SPACING)
#define LANDSCAPE_PANEL_GAP (2.0f * VOE_EDITOR_SPACING)

// What one millimetre of drag across the size box is worth, in metres.
#define LANDSCAPE_PANEL_STEP 1.0

bool voe_editor_landscape_panel_show(voe_editor_landscape_panel *panel,
				     const char *path, float size)
{
	size_t length;

	VOE_BASE_ASSERT(panel != NULL && path != NULL,
			"showing no landscape panel or no path");
	length = strlen(path);
	if (length >= sizeof panel->path)
		return false;
	// memmove, since `path` may be the panel's own.
	memmove(panel->path, path, length + 1);
	panel->size = size;
	panel->dragging = false;
	panel->showing = true;
	return true;
}

void voe_editor_landscape_panel_hide(voe_editor_landscape_panel *panel)
{
	VOE_BASE_ASSERT(panel != NULL, "hiding no landscape panel");

	panel->showing = false;
}

// A value clamped to the file's range, then rounded to a whole metre.
static float size_rounded(double value)
{
	if (value < VOE_ASSETS_LANDSCAPE_SIZE_MIN)
		value = VOE_ASSETS_LANDSCAPE_SIZE_MIN;
	if (value > VOE_ASSETS_LANDSCAPE_SIZE_MAX)
		value = VOE_ASSETS_LANDSCAPE_SIZE_MAX;
	return (float)(int)(value + 0.5);
}

void voe_editor_landscape_panel_draw(voe_ui_context *ui,
				     voe_editor_landscape_panel *panel,
				     float top, voe_math_float2 surface)
{
	float shown;

	VOE_BASE_ASSERT(ui != NULL, "drawing no landscape panel into no interface");
	VOE_BASE_ASSERT(panel != NULL, "drawing no landscape panel");
	VOE_BASE_ASSERT(panel->showing,
			"drawing a landscape panel that is not showing");
	shown = panel->dragging ? panel->dragged : panel->size;

	// Anchored exactly as project_panel.c's panel is.
	voe_ui_panel_begin(
		ui, "landscape", 0, VOE_UI_SURFACE_RAISED,
		(voe_ui_container){
			.across = VOE_UI_ACROSS_FILL,
			.gap = LANDSCAPE_PANEL_GAP,
			.pad = { LANDSCAPE_PANEL_PAD, LANDSCAPE_PANEL_PAD,
				 LANDSCAPE_PANEL_PAD, LANDSCAPE_PANEL_PAD },
			.anchor = { .anchored = true,
				    .x = { VOE_UI_ACROSS_FILL, 0.0f },
				    .y = { VOE_UI_ACROSS_START, top } },
			.size = { .across = { VOE_UI_SIZE_FIXED, surface.y } } });

	// The title and the file's name at the left, the × at the right.
	voe_ui_row_begin(ui, (voe_ui_container){ .along = VOE_UI_ALONG_SPREAD,
						 .across = VOE_UI_ACROSS_CENTER });
	voe_ui_row_begin(ui, (voe_ui_container){ .across = VOE_UI_ACROSS_CENTER,
						 .gap = LANDSCAPE_PANEL_GAP });
	voe_ui_label(ui, "Landscape");
	voe_ui_label(ui, voe_platform_path_name(panel->path));
	voe_ui_end(ui); // title and name
	panel->close_button = voe_ui_button_begin(ui, "close", 0);
	voe_ui_label(ui, "×");
	voe_ui_end(ui); // × button
	voe_ui_end(ui); // title row

	voe_ui_row_begin(ui, (voe_ui_container){ .across = VOE_UI_ACROSS_CENTER,
						 .gap = LANDSCAPE_PANEL_GAP });
	voe_ui_label(ui, "Size (m)");
	panel->size_box = voe_ui_number_begin(ui, "size", 0, (double)shown,
					      LANDSCAPE_PANEL_STEP);
	(void)snprintf(panel->size_text, sizeof panel->size_text, "%d",
		       (int)size_rounded(shown));
	voe_ui_label(ui, panel->size_text);
	voe_ui_end(ui); // number box
	voe_ui_end(ui); // size row

	voe_ui_end(ui); // panel
}

// A drag is followed in `dragged`, unrounded so slow drags add up, and handed
// back once, on its release; a typed commit is handed back at once.
voe_editor_landscape_panel_result
voe_editor_landscape_panel_clicks_read(const voe_ui_context *ui,
				       voe_editor_landscape_panel *panel)
{
	VOE_BASE_ASSERT(ui != NULL, "reading the clicks of no interface");
	VOE_BASE_ASSERT(panel != NULL, "reading the clicks of no landscape panel");

	// A refused frame hands back VOE_UI_NODE_NONE past the node budget,
	// and those are skipped, exactly as project_panel.c's are.
	voe_editor_landscape_panel_result result = { .size = panel->size };

	if (panel->size_box != VOE_UI_NODE_NONE) {
		voe_ui_number_result number =
			voe_ui_number_action(ui, panel->size_box);
		bool ended = panel->dragging && !number.held;

		if (number.held && !panel->dragging)
			panel->dragged = panel->size;
		if (number.changed)
			panel->dragged = (float)number.value;
		if (number.changed || ended)
			result.size = size_rounded(panel->dragged);
		if (number.held)
			result.size = panel->size;
		panel->dragging = number.held;
	}
	result.closed = panel->close_button != VOE_UI_NODE_NONE &&
			voe_ui_button_action(ui, panel->close_button).fired;
	result.changed = result.size != panel->size;

	return result;
}
