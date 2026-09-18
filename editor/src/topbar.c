// The bar's one frame of `ui` calls, and the read of its four buttons
// afterwards. See the header for why the command it hands back is not carried
// out here.
//
// A PANEL FOR THE BACKGROUND, A ROW FOR THE FLOW. voe_ui_panel_begin is a
// column with something drawn behind it (ui/widgets.h), and this bar's
// contents run left to right — so the panel holds one row, stretched to its
// full width and height, and everything below is that row's children.
#include "topbar.h"

#include <base/arena.h>
#include <base/assert.h>

#include <ui/widgets.h>

#include <stdio.h>

// The bar's own surface is the theme's ordinary SURFACE, the same role
// dock.c's panels use, so the bar reads as part of the one interface and not
// as a strip of something else drawn over it.

// Inside the bar's four edges, and between the things on it. Millimetres.
#define BAR_PAD 2.0f
#define BAR_GAP 3.0f

// "<name> (unsaved)", formatted into arena. Measured first and written
// second, exactly as inspector.c's own text() does: the length of a project's
// name is not something to guess at with a fixed buffer, and arena is what
// the label is allowed to point into once this call has returned.
static const char *unsaved_name(voe_base_arena *arena, const char *name)
{
	int length = snprintf(NULL, 0, "%s (unsaved)", name);
	char *out;

	VOE_BASE_ASSERT(length >= 0,
			"a project name this bar could not format");

	out = voe_base_arena_push(arena, (size_t)length + 1);
	(void)snprintf(out, (size_t)length + 1, "%s (unsaved)", name);

	return out;
}

void voe_editor_topbar_draw(voe_ui_context *ui, voe_editor_topbar *bar,
			   voe_base_arena *arena, const char *name,
			   bool unsaved, const char *notice)
{
	VOE_BASE_ASSERT(ui != NULL, "drawing no top bar into no interface");
	VOE_BASE_ASSERT(bar != NULL,
			"drawing a top bar with nowhere to record its buttons");
	VOE_BASE_ASSERT(arena != NULL, "drawing a top bar without an arena");
	VOE_BASE_ASSERT(name != NULL, "drawing a top bar with no name to show");
	VOE_BASE_ASSERT(notice != NULL,
			"drawing a top bar with no notice to show");

	voe_ui_panel_begin(
		ui, "topbar", 0, VOE_UI_SURFACE_SURFACE,
		(voe_ui_container){
			.size = { .along = { VOE_UI_SIZE_FIXED,
					     VOE_EDITOR_TOPBAR_HIGH } },
			.across = VOE_UI_ACROSS_FILL,
			.pad = { BAR_PAD, BAR_PAD, BAR_PAD, BAR_PAD } });

	voe_ui_row_begin(ui, (voe_ui_container){
				     .size = { .along = { VOE_UI_SIZE_GROW,
							  1.0f } },
				     .across = VOE_UI_ACROSS_CENTER,
				     .gap = BAR_GAP });

	bar->new_button = voe_ui_button_begin(ui, "new", 0);
	voe_ui_label(ui, "New");
	voe_ui_end(ui);

	bar->open_button = voe_ui_button_begin(ui, "open", 0);
	voe_ui_label(ui, "Open");
	voe_ui_end(ui);

	bar->save_button = voe_ui_button_begin(ui, "save", 0);
	voe_ui_label(ui, "Save");
	voe_ui_end(ui);

	bar->preferences_button = voe_ui_button_begin(ui, "preferences", 0);
	voe_ui_label(ui, "Preferences");
	voe_ui_end(ui);

	voe_ui_label(ui, unsaved ? unsaved_name(arena, name) : name);

	if (notice[0] != '\0')
		voe_ui_label(ui, notice);

	voe_ui_end(ui); // row
	voe_ui_end(ui); // panel
}

voe_editor_command voe_editor_topbar_clicks_read(const voe_ui_context *ui,
						 const voe_editor_topbar *bar)
{
	VOE_BASE_ASSERT(ui != NULL, "reading the clicks of no interface");
	VOE_BASE_ASSERT(bar != NULL, "reading the clicks of no top bar");

	// A refused frame hands back VOE_UI_NODE_NONE for every widget past
	// the node budget, and asking one of those what the pointer did is
	// the caller's bug — so they are skipped, exactly as the Scene
	// panel's rows are (scene.c): a full frame is `ui`'s to report and
	// not this file's to fail on.
	if (bar->new_button != VOE_UI_NODE_NONE &&
	    voe_ui_button_action(ui, bar->new_button).fired)
		return VOE_EDITOR_COMMAND_NEW;

	if (bar->open_button != VOE_UI_NODE_NONE &&
	    voe_ui_button_action(ui, bar->open_button).fired)
		return VOE_EDITOR_COMMAND_OPEN;

	if (bar->save_button != VOE_UI_NODE_NONE &&
	    voe_ui_button_action(ui, bar->save_button).fired)
		return VOE_EDITOR_COMMAND_SAVE;

	return VOE_EDITOR_COMMAND_NONE;
}

bool voe_editor_topbar_preferences_read(const voe_ui_context *ui,
					const voe_editor_topbar *bar)
{
	VOE_BASE_ASSERT(ui != NULL, "reading the clicks of no interface");
	VOE_BASE_ASSERT(bar != NULL, "reading the clicks of no top bar");

	return bar->preferences_button != VOE_UI_NODE_NONE &&
	       voe_ui_button_action(ui, bar->preferences_button).fired;
}
