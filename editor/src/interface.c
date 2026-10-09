// One `ui` frame per root, submitted into the open frame and drawn with
// voe_render_element_transform. See the header for why a root is a frame and why
// the pointer arrives already in millimetres.
//
// NOTHING IN HERE DECIDES WHAT IS ON A PANEL. It opens the frame, hands over the
// pointer it was given, asks the dock to walk, and moves records into the
// device; what a panel says is dock.c's `voe_editor_panel_draw`. It also polls
// Play and Ship once a frame and draws a Scene list or Assets drag's ghost.
//
// IT DOES ASK ONE QUESTION ABOUT WHAT WAS CLICKED, AND THAT IS NOT THE SAME
// THING. A `ui` widget answers what the pointer did to it only after
// voe_ui_frame_end and only while the arena its nodes came out of still holds
// them — a window this function opens and closes. So the one read of the
// frame's clicks is called here, between the two: the scene's and the
// commands' (interface_read.h), and the Assets panel's (interface_assets.h).
//
// THE COLOUR PICKER IS DRAWN HERE TOO. While scene.h's `picking` names the
// selected entity and that entity still has the row, the picker sits anchored
// over the dock just left of the Inspector column, seeded from the row; its
// result is read by interface_read.c. The browser and Preferences cover the
// same area, and either showing closes it.
//
// THE OPEN DROPDOWN'S LIST IS THE INSPECTOR'S OWN (inspector.h) AND NOT THIS
// FILE'S. It is drawn inside that panel so that it moves and disappears with the
// button it hangs from (ADR-0199), and the only thing this file still does to it
// is close it on Escape.
//
// THE TOP BAR'S PANELS LIST (panels_menu.h) IS DRAWN HERE, LAST, over every
// other panel, while the bar's `menu` is open; Escape or the browser showing
// close it. THE ASSETS PANEL'S RIGHT-BUTTON MENU (assets_menu.h) is drawn here
// too, Escape and the dock's covers closing it.
#include "interface.h"

#include "assets_ask.h"
#include "assets_menu.h"
#include "browser.h"
#include "errors.h"
#include "inspector.h"
#include "inspector_edit.h"
#include "inspector_sculpt.h"
#include "interface_assets.h"
#include "interface_read.h"
#include "landscape_panel.h"
#include "notice.h"
#include "panels.h"
#include "panels_menu.h"
#include "preferences.h"
#include "project.h"
#include "project_panel.h"
#include "scene_list.h"
#include "themes.h"
#include "topbar.h"

#include <base/assert.h>

#include <platform/path.h>

#include <ui/colour.h>
#include <ui/theme.h>

// Between the picker and the Inspector column, and below the bar. Millimetres.
#define PICKER_GAP (1.0f * VOE_EDITOR_SPACING)

voe_ui_context *voe_editor_interface_new(voe_base_arena *arena,
					 const voe_ui_theme *theme)
{
	voe_ui_context *ui;

	VOE_BASE_ASSERT(arena != NULL, "making an interface without an arena");
	VOE_BASE_ASSERT(theme != NULL && theme->font != NULL,
			"an interface with no theme or no font");

	ui = voe_ui_context_new(
		arena, (voe_ui_capacities){
			       .nodes = VOE_EDITOR_INTERFACE_NODES,
			       .elements = VOE_EDITOR_INTERFACE_ELEMENTS,
			       .scrolls = VOE_EDITOR_INTERFACE_SCROLLS });
	voe_ui_font_set(ui, theme->font);
	voe_ui_theme_set(ui, theme);

	return ui;
}

void voe_editor_interface_surface(voe_platform_size target,
				  voe_math_float2 *millimetres,
				  float *pixels_per_millimetre)
{
	float scale;

	VOE_BASE_ASSERT(millimetres != NULL,
			"asking how big the surface is with nowhere to put the answer");
	VOE_BASE_ASSERT(pixels_per_millimetre != NULL,
			"asking what a millimetre is worth with nowhere to put the answer");
	VOE_BASE_ASSERT(target.width > 0 && target.height > 0,
			"a surface on a window with no area");

	scale = (float)target.height / VOE_EDITOR_SURFACE_HIGH *
		VOE_EDITOR_UI_SCALE;

	*millimetres = voe_render_element_surface_size(target, scale);
	*pixels_per_millimetre = scale;
}

// The order of the reads after voe_ui_frame_end is what makes them agree: the
// scene's first (interface_read.h), then the Assets panel's
// (interface_assets.h), the views' rectangles, and the commands, whose browser
// or session may replace the world, last.
bool voe_editor_interface_draw(voe_render_device *gpu, voe_ui_context *ui,
			       voe_base_arena *arena,
			       voe_editor_dock_root *roots,
			       uint32_t count, voe_editor_scene *scene,
			       const voe_editor_assets_drag *drag,
			       voe_editor_views *views,
			       voe_editor_undo *undo,
			       voe_editor_models *models,
			       voe_editor_session *session,
			       voe_editor_topbar *bar,
			       voe_editor_browser *browser,
			       voe_editor_preferences *preferences,
			       voe_editor_project_panel *project_panel,
			       voe_editor_landscape_panel *landscape_panel,
			       voe_editor_themes *themes,
			       voe_editor_frame_breakdown *breakdown, bool escape)
{
	struct voe_base_arena_mark mark;
	bool ok = true;

	VOE_BASE_ASSERT(gpu != NULL, "drawing the interface on no device");
	VOE_BASE_ASSERT(ui != NULL, "drawing no interface");
	VOE_BASE_ASSERT(arena != NULL, "drawing the interface without an arena");
	VOE_BASE_ASSERT(roots != NULL, "drawing an interface with no roots");
	VOE_BASE_ASSERT(scene != NULL, "drawing an interface with no scene");
	VOE_BASE_ASSERT(drag != NULL, "drawing an interface with no drag");
	VOE_BASE_ASSERT(views != NULL, "drawing an interface with no views");
	VOE_BASE_ASSERT(undo != NULL, "drawing an interface with no undo line");
	VOE_BASE_ASSERT(models != NULL, "drawing an interface with no models");
	VOE_BASE_ASSERT(session != NULL, "drawing an interface with no session");
	VOE_BASE_ASSERT(bar != NULL, "drawing an interface with no top bar");
	VOE_BASE_ASSERT(browser != NULL, "drawing an interface with no browser");
	VOE_BASE_ASSERT(preferences != NULL,
			"drawing an interface with no preferences");
	VOE_BASE_ASSERT(project_panel != NULL,
			"drawing an interface with no project panel");
	VOE_BASE_ASSERT(landscape_panel != NULL,
			"drawing an interface with no landscape panel");
	VOE_BASE_ASSERT(themes != NULL, "drawing an interface with no themes");
	VOE_BASE_ASSERT(breakdown != NULL,
			"drawing an interface with no frame breakdown");

	// Once a frame, before any root is built, so every root's bar reads
	// the same label (interface.h).
	voe_editor_session_play_poll(session);
	voe_editor_session_ship_poll(session);

	for (uint32_t i = 0; i < count && ok; i++) {
		voe_editor_dock_root *root = &roots[i];
		// Each header's ×, recorded by the walk (dock.h).
		voe_editor_dock_closes closes;
		// The dock tree's own root, its height cut down by the bar
		// above it — dock.c's own tree is untouched, only the size
		// its walk divides out.
		voe_editor_dock_root below_bar = *root;
		// The open prefab's file in the project's place (0283 point 8).
		bool prefab_open = session->project->prefab[0] != '\0';
		const char *name =
			prefab_open ?
				voe_platform_path_name(session->project->prefab) :
				voe_editor_project_name(session->project);
		uint32_t first;
		uint32_t records;
		// Captured before anything is drawn, and used for every
		// decision below instead of reading browser->showing again —
		// see the header on why a click read after this frame's own
		// commands have run has to answer for what this frame
		// actually laid out.
		bool browsing = browser->showing;
		// The same, for the Errors panel, which the browser covers,
		// and for preferences, which both cover.
		bool erroring = session->errors.showing && !browsing;
		bool preferring = preferences->showing && !browsing &&
				  !erroring;
		// The Project panel, in the same place, under all three.
		bool projecting = project_panel->showing && !browsing &&
				  !erroring && !preferring;
		// The Landscape panel, in the same place, under all four.
		bool landscaping = landscape_panel->showing && !browsing &&
				   !erroring && !preferring && !projecting;
		// The frame breakdown, under all five, the dock still live.
		bool framing = breakdown->showing && !browsing && !erroring &&
			       !preferring && !projecting && !landscaping;
		// Any of the panels drawn over the dock.
		bool covered = browsing || preferring || erroring ||
			       projecting || landscaping;
		// The Assets panel's Delete question, over all of those but
		// the browser, whose showing closes it.
		bool asking;
		// The Panels list, closed by Escape or the browser showing.
		bool menuing;
		// The Assets panel's right-button menu (assets_menu.h).
		bool assets_menuing;
		// The picker, when it shows, and what it edits: the target as
		// it was when drawn, whatever this frame's clicks do to it.
		voe_math_float3 colour;
		voe_editor_picking picked;
		voe_ui_node picker = VOE_UI_NODE_NONE;
		bool picking;
		// Whether the pointer is over the Assets leaf, for the Scene
		// list's drop and the Assets panel's keyboard.
		bool over_assets;

		if (covered)
			voe_editor_scene_picker_close(scene);
		if (browsing)
			voe_editor_assets_ask_close(&session->asking);
		asking = session->asking.open;
		// Any panel over the dock closes the Assets menu; Escape closes
		// it first and goes no further.
		if (covered || asking)
			voe_editor_assets_menu_close(&scene->assets.menu);
		if (escape && scene->assets.menu.open) {
			voe_editor_assets_menu_close(&scene->assets.menu);
			escape = false;
		}
		assets_menuing = scene->assets.menu.open;
		picking = voe_editor_scene_picker_showing(scene, &colour);
		picked = scene->picking;
		// The open list and Add component's are the Inspector's own
		// (inspector.h); closing them on Escape is all this file does.
		if (escape) {
			voe_editor_scene_dropdown_close(scene);
			voe_editor_inspector_add_close(&scene->inspector);
		}

		if (browsing || escape)
			bar->menu.open = false;
		menuing = bar->menu.open;

		below_bar.size.y -= voe_editor_topbar_high(bar, root->size.y);
		if (covered || menuing || asking)
			below_bar.pointer.over = false;

		// The tree lives in the arena only until its records have been
		// read out of it, which is before the next root is walked.
		mark = voe_base_arena_mark(arena);

		voe_ui_frame_begin(ui, arena);
		voe_ui_pointer_set(ui, root->pointer);
		// BESIDE THE POINTER AND FOR THE SAME REASON (dock.h, task 14):
		// this frame's typing, wherever main.c read it, so the
		// browser's own name field (browser.h) can be typed into
		// without this file naming one.
		voe_ui_keyboard_set(ui, root->keyboard);
		// The inspector formats every label it draws into this arena
		// and hands back its controls through nodes out of this frame,
		// so it is opened here beside the frame and not inside the walk.
		voe_editor_inspector_frame_begin(&scene->inspector, arena,
						 &scene->dropdown);
		voe_editor_inspector_sculpt_forget(&scene->sculpt);

		// ONE COLUMN IS THIS FRAME'S ROOT, AND THE BAR AND THE TREE ARE
		// ITS TWO CHILDREN. voe_ui_frame_begin requires the very first
		// call to open the root (ui/layout.h); the tree's own row,
		// opened inside voe_editor_dock_walk, is a nested child of it
		// rather than the root itself — WHICH IS WHY THAT CALL IS
		// GIVEN VOE_EDITOR_DOCK_COLUMN BELOW. A child's own size is
		// read against its PARENT's flow and not its own
		// (ui/layout.h), so dock.c's row has to be told it is inside
		// a column now rather than being the frame's actual root, or
		// its width and height come out swapped (dock.h).
		voe_ui_column_begin(
			ui, (voe_ui_container){
				    .size = { .along = { VOE_UI_SIZE_FIXED,
							 root->size.y },
					      .across = { VOE_UI_SIZE_FIXED,
							  root->size.x } },
				    .across = VOE_UI_ACROSS_FILL });
		voe_editor_topbar_draw(ui, bar, arena, root->size.y,
				       voe_editor_session_play_label(session),
				       voe_editor_refresh_label(&session->refresh),
				       voe_editor_session_ship_label(session),
				       scene->rings ? "Rotate" : "Move",
				       name != NULL ? name : "Untitled",
				       session->project->unsaved,
				       session->notice.text, prefab_open);
		voe_editor_dock_walk(&below_bar, VOE_EDITOR_DOCK_COLUMN, ui,
				     &voe_editor_themes_chosen(themes)->palette,
				     scene, views, &closes);
		// A Scene list or Assets drag's ghost, anchored beside the
		// pointer (ADR-0282, 0286); before the overlays, so they paint
		// over it.
		voe_editor_scene_list_ghost_draw(ui, scene, root->pointer.at);
		voe_editor_assets_drag_ghost_draw(ui, &scene->list_dim, drag,
						  root->pointer.at);
		// ANCHORED, SO ITS PLACE IN THIS CALL ORDER DOES NOT MATTER TO
		// WHERE IT PAINTS (ui/layout.h) — it is called here, after the
		// tree, only because that is where browser.h's own state (the
		// dock's below it) is settled.
		if (browsing)
			voe_editor_browser_draw(ui, browser,
						voe_editor_topbar_high(
							bar, root->size.y),
						below_bar.size);
		if (preferring)
			voe_editor_preferences_draw(ui, preferences, themes,
						    voe_editor_topbar_high(
							    bar, root->size.y),
						    below_bar.size);
		if (projecting)
			voe_editor_project_panel_draw(
				ui, project_panel, session->project->file.window,
				voe_editor_topbar_high(bar, root->size.y),
				below_bar.size);
		if (landscaping)
			voe_editor_landscape_panel_draw(
				ui, landscape_panel,
				voe_editor_topbar_high(bar, root->size.y),
				below_bar.size);
		if (erroring)
			voe_editor_errors_draw(ui, &session->errors,
					       voe_editor_topbar_high(
						       bar, root->size.y),
					       below_bar.size);
		// At the left under the bar, over the Scene list.
		if (framing)
			voe_editor_frame_breakdown_draw(
				ui, breakdown,
				(voe_math_float2){
					PICKER_GAP,
					voe_editor_topbar_high(bar, root->size.y) +
						PICKER_GAP });
		if (asking)
			voe_editor_assets_ask_draw(
				ui, &session->asking,
				voe_editor_topbar_high(bar, root->size.y));
		// Its right edge PICKER_GAP short of the Inspector's content,
		// its top PICKER_GAP under the bar; the column around it only
		// carries the anchor, the picker being a panel of its own.
		if (picking) {
			voe_ui_column_begin(
				ui,
				(voe_ui_container){
					.anchor = {
						.anchored = true,
						.x = { VOE_UI_ACROSS_END,
						       root->size.x -
							       picked.left +
							       PICKER_GAP },
						.y = { VOE_UI_ACROSS_START,
						       voe_editor_topbar_high(
							       bar,
							       root->size.y) +
							       PICKER_GAP } } });
			picker = voe_ui_colour_picker(ui, "colour picker", 0,
						      colour);
			voe_ui_end(ui);
		}
		// The Panels list last, so it paints over every other panel,
		// hanging below where Panels sat last frame, ticked as now.
		if (menuing) {
			bool open[VOE_EDITOR_CLOSABLE_COUNT];

			for (uint32_t c = 0; c < VOE_EDITOR_CLOSABLE_COUNT; c++)
				open[c] = voe_editor_panels_open(
					(voe_editor_closable)c, root,
					project_panel, session, breakdown);
			voe_editor_panels_menu_draw(ui, &bar->menu,
						    bar->panels_under, open);
		}
		// The Assets menu over everything too; it is never open beside
		// the Panels list, a press on either closing the other.
		if (assets_menuing)
			voe_editor_assets_menu_draw(ui, &scene->assets.menu);
		voe_ui_end(ui);

		if (!voe_ui_frame_end(ui)) {
			voe_base_arena_rewind(arena, mark);
			return false;
		}

		// Before the rewind below, which is the whole of the window a
		// widget will answer in. A refused frame above is not asked at
		// all: nothing was laid out, so nothing was clicked.
		over_assets = voe_editor_interface_scene_read(
			ui, arena, root, scene, session, bar, picker, &picked);
		voe_editor_interface_assets_read(
			ui, arena, root, scene, undo, models, session, browser,
			preferences, project_panel, landscape_panel,
			over_assets, assets_menuing, asking);
		voe_editor_views_rects_read(views, ui);
		voe_editor_interface_commands_read(
			ui, arena, root, &closes, scene, models, session, bar,
			browser, preferences, project_panel, landscape_panel,
			themes, breakdown,
			(voe_editor_interface_shown){
				.browsing = browsing,
				.erroring = erroring,
				.preferring = preferring,
				.projecting = projecting,
				.landscaping = landscaping,
				.framing = framing,
				.menuing = menuing },
			escape);

		// The range this root fills, read either side of its own
		// submissions: there is no id and nothing allocated, and
		// another root's records are another range of the same buffer.
		first = voe_render_frame_elements_submitted(gpu);
		records = voe_ui_element_count(ui);
		for (uint32_t e = 0; e < records && ok; e++)
			ok = voe_render_frame_submit_element(
				gpu, voe_ui_element(ui, e));

		// ONE COMMAND FOR THE WHOLE ROOT, WHATEVER IS ON IT. Not one per
		// panel and not one per letter: every record in the range is the
		// same eighty bytes in the same buffer.
		if (ok)
			ok = voe_render_frame_draw_elements(
				gpu, voe_render_element_transform(root->size),
				first, records);

		voe_base_arena_rewind(arena, mark);
	}

	return ok;
}
