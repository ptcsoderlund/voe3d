// The bar's one frame of `ui` calls, and the read of its buttons, Back among
// them while a prefab is open, afterwards. See the header for why the command it hands back is not carried
// out here.
//
// A PANEL FOR THE BACKGROUND, A ROW FOR THE FLOW. voe_ui_panel_begin is a
// column with something drawn behind it (ui/widgets.h), and this bar's
// contents run left to right — so the panel holds one row, stretched to its
// full width at its natural height, and everything below is that row's
// children. The panel is fixed at voe_editor_topbar_high; the row is
// not, so the panel's measure is what its content needs (ADR-0225).
#include "topbar.h"

#include "dock.h"

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
			   voe_base_arena *arena, float surface_high,
			   const char *play, const char *refresh,
			   const char *ship, const char *gizmo,
			   const char *name, bool unsaved, const char *notice,
			   bool back)
{
	VOE_BASE_ASSERT(ui != NULL, "drawing no top bar into no interface");
	VOE_BASE_ASSERT(bar != NULL,
			"drawing a top bar with nowhere to record its buttons");
	VOE_BASE_ASSERT(arena != NULL, "drawing a top bar without an arena");
	VOE_BASE_ASSERT(play != NULL, "drawing a top bar with no Play label");
	VOE_BASE_ASSERT(refresh != NULL,
			"drawing a top bar with no Refresh label");
	VOE_BASE_ASSERT(ship != NULL, "drawing a top bar with no Ship label");
	VOE_BASE_ASSERT(gizmo != NULL, "drawing a top bar with no gizmo mode");
	VOE_BASE_ASSERT(name != NULL, "drawing a top bar with no name to show");
	VOE_BASE_ASSERT(notice != NULL,
			"drawing a top bar with no notice to show");

	bar->panel = voe_ui_panel_begin(
		ui, "topbar", 0, VOE_UI_SURFACE_SURFACE,
		(voe_ui_container){
			.size = { .along = { VOE_UI_SIZE_FIXED,
					     voe_editor_topbar_high(
						     bar, surface_high) } },
			.across = VOE_UI_ACROSS_FILL,
			.pad = { BAR_PAD, BAR_PAD, BAR_PAD, BAR_PAD } });

	voe_ui_row_begin(ui, (voe_ui_container){
				     .across = VOE_UI_ACROSS_CENTER,
				     .gap = BAR_GAP });

	bar->back_button = VOE_UI_NODE_NONE;
	if (back) {
		bar->back_button = voe_ui_button_begin(ui, "back", 0);
		voe_ui_label(ui, "Back");
		voe_ui_end(ui);
	}

	bar->new_button = voe_ui_button_begin(ui, "new", 0);
	voe_ui_label(ui, "New");
	voe_ui_end(ui);

	bar->open_button = voe_ui_button_begin(ui, "open", 0);
	voe_ui_label(ui, "Open");
	voe_ui_end(ui);

	bar->save_button = voe_ui_button_begin(ui, "save", 0);
	voe_ui_label(ui, "Save");
	voe_ui_end(ui);

	bar->play_button = voe_ui_button_begin(ui, "play", 0);
	voe_ui_label(ui, play);
	voe_ui_end(ui);

	bar->refresh_button = voe_ui_button_begin(ui, "refresh", 0);
	voe_ui_label(ui, refresh);
	voe_ui_end(ui);

	bar->ship_button = voe_ui_button_begin(ui, "ship", 0);
	voe_ui_label(ui, ship);
	voe_ui_end(ui);

	bar->preferences_button = voe_ui_button_begin(ui, "preferences", 0);
	voe_ui_label(ui, "Preferences");
	voe_ui_end(ui);

	voe_ui_label(ui, gizmo);
	voe_ui_label(ui, unsaved ? unsaved_name(arena, name) : name);

	if (notice[0] != '\0')
		voe_ui_label(ui, notice);

	voe_ui_end(ui); // row
	voe_ui_end(ui); // panel
}

float voe_editor_topbar_least(const voe_editor_topbar *bar)
{
	float high;

	VOE_BASE_ASSERT(bar != NULL, "asking the height of no top bar");

	high = bar->high > 0.0f ? bar->high : VOE_EDITOR_TOPBAR_HIGH;
	VOE_BASE_ASSERT(high > 0.0f, "a top bar with no height");

	return high;
}

float voe_editor_topbar_high(const voe_editor_topbar *bar, float surface_high)
{
	float high = voe_editor_topbar_least(bar);

	if (bar->wanted > high)
		high = bar->wanted;
	if (high > surface_high - VOE_EDITOR_DOCK_VIEW_ROOM)
		high = surface_high - VOE_EDITOR_DOCK_VIEW_ROOM;
	return high > 0.0f ? high : 0.0f;
}

void voe_editor_topbar_measure(const voe_ui_context *ui,
			       voe_editor_topbar *bar)
{
	VOE_BASE_ASSERT(ui != NULL, "measuring a top bar in no interface");
	VOE_BASE_ASSERT(bar != NULL, "measuring no top bar");

	if (bar->panel != VOE_UI_NODE_NONE)
		bar->high = voe_ui_node_measured(ui, bar->panel).y;
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
	if (bar->back_button != VOE_UI_NODE_NONE &&
	    voe_ui_button_action(ui, bar->back_button).fired)
		return VOE_EDITOR_COMMAND_BACK;

	if (bar->new_button != VOE_UI_NODE_NONE &&
	    voe_ui_button_action(ui, bar->new_button).fired)
		return VOE_EDITOR_COMMAND_NEW;

	if (bar->open_button != VOE_UI_NODE_NONE &&
	    voe_ui_button_action(ui, bar->open_button).fired)
		return VOE_EDITOR_COMMAND_OPEN;

	if (bar->save_button != VOE_UI_NODE_NONE &&
	    voe_ui_button_action(ui, bar->save_button).fired)
		return VOE_EDITOR_COMMAND_SAVE;

	if (bar->play_button != VOE_UI_NODE_NONE &&
	    voe_ui_button_action(ui, bar->play_button).fired)
		return VOE_EDITOR_COMMAND_PLAY;

	if (bar->refresh_button != VOE_UI_NODE_NONE &&
	    voe_ui_button_action(ui, bar->refresh_button).fired)
		return VOE_EDITOR_COMMAND_REFRESH;

	if (bar->ship_button != VOE_UI_NODE_NONE &&
	    voe_ui_button_action(ui, bar->ship_button).fired)
		return VOE_EDITOR_COMMAND_SHIP;

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
