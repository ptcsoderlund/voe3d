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
// them — a window this function opens and closes. So the one line that reads
// the frame's clicks is here, between the two, and what a click MEANS is
// scene.c's — EXCEPT FOR THE TOP BAR'S, THE BROWSER'S, PREFERENCES' AND THE
// ERRORS PANEL'S OWN, whose clicks are
// commands carried out right here, through voe_editor_session_do and
// voe_editor_session_browser_do, because the same window is the only place
// topbar.h's and browser.h's recorded buttons can be asked either. Only one of
// the two is ever read in a given frame — see voe_editor_interface_draw's own
// header on `browsing`. Preferences' Choose is carried out here too, through
// themes.h, and its palette set on the context for the next frame; the
// Project panel's changed window is written through project.h.
//
// THE COLOUR PICKER IS DRAWN HERE TOO, AND ITS RESULT READ HERE, for the same
// reason: it is a `ui` widget answering after voe_ui_frame_end. While scene.h's
// `picking` names the selected entity and that entity still has the row, the
// picker sits anchored over the dock just left of the Inspector column, seeded
// from the row. A `changed` goes through inspector.h's
// voe_editor_inspector_colour_submit at once, so the shape changes live; an
// `outside` press closes it. It is read before the Inspector's buttons, so a
// swatch that fires in the frame an outside press closed the picker opens it
// again rather than being closed behind. The browser and Preferences cover the
// same area, and either showing closes it.
//
// THE SCENE LIST'S DROP IS CARRIED OUT IN THAT SAME ONE READ, through
// scene_list.h's voe_editor_scene_list_drop, after the rows' clicks. Released
// over the Assets leaf it makes a prefab (prefabs.h), one structural change;
// whether the Assets leaf takes it is !voe_editor_prefab_make_refused for the
// session's project, which the ghost shows as refused or not. Then scene.h's
// reveal shows a selection made elsewhere in the list.
//
// A PREFAB ROW FIRED IN THE ASSETS PANEL OPENS IT (session.h), beside where
// Import shows the browser; the panel's naming request is carried out through
// assets_manage.h in the frame's arena, a rename's typed `/` refused here.
// Its Delete request opens the session's question (assets_ask.h), drawn over
// the dock and read here: Delete trashes through assets_manage.h, and it or
// Cancel or a press outside closes it.
//
// THE OPEN DROPDOWN'S LIST IS THE INSPECTOR'S OWN (inspector.h) AND NOT THIS
// FILE'S. It is drawn inside that panel so that it moves and disappears with the
// button it hangs from (ADR-0199), and the only thing this file still does to it
// is close it on Escape.
//
// THE TOP BAR'S PANELS LIST (panels_menu.h) IS DRAWN HERE, LAST, over every
// other panel, while the bar's `menu` is open; Panels flips it, a fired row
// goes through panels.h's voe_editor_panels_toggle and closes it, and Escape,
// a press off the list and Panels, or the browser showing close it. A fired
// panel header's × goes through the same toggle on its root. THE ASSETS
// PANEL'S RIGHT-BUTTON MENU (assets_menu.h) is drawn after it and read after
// the panel: Rename, Delete and Folder begin on the panel, Duplicate goes
// through assets_manage.h; Escape closes it first, as do the dock's covers.
#include "interface.h"

#include "assets_ask.h"
#include "assets_manage.h"
#include "assets_menu.h"
#include "browser.h"
#include "errors.h"
#include "inspector.h"
#include "inspector_edit.h"
#include "inspector_place.h"
#include "notice.h"
#include "panels.h"
#include "panels_menu.h"
#include "prefabs.h"
#include "preferences.h"
#include "project.h"
#include "project_panel.h"
#include "scene_list.h"
#include "themes.h"
#include "topbar.h"

#include <base/assert.h>
#include <base/report.h>

#include <platform/path.h>

#include <ui/colour.h>
#include <ui/theme.h>

#include <string.h>

// Between the picker and the Inspector column, and below the bar. Millimetres.
#define PICKER_GAP (1.0f * VOE_EDITOR_SPACING)

// The Assets panel's request carried out and cleared. A rename's `to` is the
// shown folder joined with the typed name, so a `/` in it would move the file
// into a folder rather than rename it (assets_manage.h's constraint); it is
// refused here in the words assets_manage.c uses for the other separators.
static void assets_request_do(voe_editor_session *session,
			      voe_editor_scene *scene, voe_editor_undo *undo,
			      voe_base_arena *arena)
{
	voe_editor_assets_request *request = &scene->assets.request;

	VOE_BASE_ASSERT(session != NULL && scene != NULL && undo != NULL,
			"an Assets request with no session, scene or undo");
	VOE_BASE_ASSERT(arena != NULL, "an Assets request with no scratch");
	if (request->kind == VOE_EDITOR_ASSETS_NAMING_RENAME) {
		if (strchr(request->name, '/') != NULL)
			voe_editor_notice_set(&session->notice,
					      "%s: a name cannot hold /, \\ or \"",
					      request->name);
		else
			// A false has said why in the notice.
			(void)voe_editor_assets_move(session, scene, undo,
						     arena, request->from,
						     request->to);
	} else if (request->kind == VOE_EDITOR_ASSETS_NAMING_FOLDER) {
		(void)voe_editor_assets_folder_make(session, scene, undo, arena,
						    request->folder,
						    request->name);
	}
	request->kind = VOE_EDITOR_ASSETS_NAMING_NONE;
}

// A fired row of the Assets menu carried out on the selected row or the shown
// folder (assets_panel.h, assets_manage.h).
static void assets_menu_do(voe_editor_session *session, voe_editor_scene *scene,
			   voe_editor_undo *undo, voe_base_arena *arena,
			   voe_editor_assets_menu_item item)
{
	char path[VOE_EDITOR_ASSETS_PATH];

	VOE_BASE_ASSERT(session != NULL && scene != NULL && undo != NULL,
			"an Assets menu row with no session, scene or undo");
	VOE_BASE_ASSERT(arena != NULL, "an Assets menu row with no scratch");
	if (item == VOE_EDITOR_ASSETS_MENU_RENAME)
		voe_editor_assets_rename_begin(&scene->assets);
	else if (item == VOE_EDITOR_ASSETS_MENU_DELETE)
		voe_editor_assets_delete_begin(&scene->assets);
	else if (item == VOE_EDITOR_ASSETS_MENU_FOLDER)
		voe_editor_assets_folder_begin(&scene->assets);
	else if (item == VOE_EDITOR_ASSETS_MENU_DUPLICATE &&
		 voe_editor_assets_selected_path(&scene->assets, path,
						 sizeof path))
		// A false has said why in the notice.
		(void)voe_editor_assets_duplicate(session, scene, undo, arena,
						  path);
}

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

bool voe_editor_interface_draw(voe_render_device *gpu, voe_ui_context *ui,
			       voe_base_arena *arena,
			       voe_editor_dock_root *roots,
			       uint32_t count, voe_editor_scene *scene,
			       const voe_editor_assets_drag *drag,
			       voe_editor_views *views,
			       voe_editor_undo *undo,
			       voe_editor_session *session,
			       voe_editor_topbar *bar,
			       voe_editor_browser *browser,
			       voe_editor_preferences *preferences,
			       voe_editor_project_panel *project_panel,
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
	VOE_BASE_ASSERT(session != NULL, "drawing an interface with no session");
	VOE_BASE_ASSERT(bar != NULL, "drawing an interface with no top bar");
	VOE_BASE_ASSERT(browser != NULL, "drawing an interface with no browser");
	VOE_BASE_ASSERT(preferences != NULL,
			"drawing an interface with no preferences");
	VOE_BASE_ASSERT(project_panel != NULL,
			"drawing an interface with no project panel");
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
		voe_editor_closable fired;
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
		// The frame breakdown, under all four, the dock still live.
		bool framing = breakdown->showing && !browsing && !erroring &&
			       !preferring && !projecting;
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
		// Why a Scene list drag over Assets would be refused, never
		// shown: the release's make says it in the session's notice.
		voe_editor_notice unshown = { 0 };
		// Whether the pointer is over the Assets leaf, for the Scene
		// list's drop and the Assets panel's keyboard.
		bool over_assets;

		if (browsing || preferring || erroring || projecting)
			voe_editor_scene_picker_close(scene);
		if (browsing)
			voe_editor_assets_ask_close(&session->asking);
		asking = session->asking.open;
		// Any panel over the dock closes the Assets menu; Escape closes
		// it first and goes no further.
		if (browsing || preferring || erroring || projecting || asking)
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
		if (browsing || preferring || erroring || projecting ||
		    menuing || asking)
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
		//
		// The edits go first, and which order they are in is a fact and
		// not a taste: a click read here can move the selection, and the
		// controls above were drawn for whatever was selected when the
		// frame was built. The inspector keeps that entity itself, so
		// the order is belt as well as braces.
		voe_editor_inspector_edits_read(&scene->inspector, ui,
						scene->world);
		if (picker != VOE_UI_NODE_NONE) {
			voe_ui_colour_result result =
				voe_ui_colour_picker_action(ui, picker);

			if (result.changed)
				voe_editor_inspector_colour_submit(
					&scene->inspector, scene->world,
					picked.entity, picked.type,
					picked.offset, result.value);
			if (result.outside)
				voe_editor_scene_picker_close(scene);
		}
		voe_editor_inspector_buttons_read(&scene->inspector, ui, scene,
						  root->pointer.down,
						  root->pointer.at);
		if (!voe_editor_scene_clicks_read(scene, ui))
			voe_editor_notice_set(&session->notice,
					      "The scene is full.");
		over_assets = voe_editor_dock_over_panel(
			root, voe_editor_topbar_high(bar, root->size.y),
			VOE_EDITOR_PANEL_ASSETS, root->pointer.at);
		voe_editor_scene_list_drop(
			scene, ui, root->pointer.down, root->pointer.at,
			over_assets,
			!voe_editor_prefab_make_refused(session->project,
							scene->list_held,
							&unshown));
		if (scene->list_made.generation != 0) {
			if (voe_editor_prefab_make(session->project,
						   scene->list_made,
						   scene->assets.shown, arena,
						   &session->notice))
				scene->structural++;
			scene->list_made = (voe_ecs_entity){ 0 };
		}
		// After every read that can move the selection this frame.
		voe_editor_scene_reveal(scene, ui);
		if (voe_editor_assets_clicks_read(ui, &scene->assets,
						  root->pointer.down,
						  over_assets))
			voe_editor_browser_show(browser,
						VOE_EDITOR_BROWSER_IMPORT, NULL,
						&session->notice);
		assets_request_do(session, scene, undo, arena);
		// After the panel's read, so a naming begun here is not ended by
		// a field it has not drawn yet, and before the question opens
		// from `deleting`.
		if (assets_menuing)
			assets_menu_do(session, scene, undo, arena,
				       voe_editor_assets_menu_read(
					       ui, &scene->assets.menu,
					       root->pointer.at,
					       root->pointer.down, root->size));
		// The question as it was drawn answered before a new one opens,
		// so a question opened this frame is not read against nodes it
		// never had.
		if (asking) {
			voe_editor_assets_ask_answer answer =
				voe_editor_assets_ask_read(ui, &session->asking,
							   root->pointer.down,
							   root->pointer.at);

			// A false has said why in the notice.
			if (answer == VOE_EDITOR_ASSETS_ASK_DELETE)
				(void)voe_editor_assets_trash(
					session, scene, undo, arena,
					session->asking.path);
			if (answer != VOE_EDITOR_ASSETS_ASK_NONE)
				voe_editor_assets_ask_close(&session->asking);
		}
		if (scene->assets.deleting[0] != '\0') {
			if (!session->asking.open)
				voe_editor_assets_ask_open(&session->asking,
							   session->project->folder,
							   scene->assets.deleting,
							   arena);
			scene->assets.deleting[0] = '\0';
		}
		if (scene->assets.opened[0] != '\0') {
			voe_editor_session_prefab_open(session, scene,
						       scene->assets.opened);
			scene->assets.opened[0] = '\0';
		}
		voe_editor_views_rects_read(views, ui);
		// A fired × closes its panel on this root, laid out so from the
		// next frame, and remembers it as a drag's end does (main.c).
		fired = voe_editor_dock_closes_read(ui, &closes);
		// A row of the Panels list fired is toggled the same way and
		// closes the list; so does a press off the list and Panels.
		if (menuing) {
			bool over;
			voe_editor_closable chosen = voe_editor_panels_menu_read(
				ui, &bar->menu, root->pointer.at, &over);

			if (chosen != VOE_EDITOR_CLOSABLE_COUNT) {
				fired = chosen;
				bar->menu.open = false;
			} else if (root->pointer.down && !over &&
				   (bar->panels_button == VOE_UI_NODE_NONE ||
				    !voe_editor_inspector_rect_contains(
					    voe_ui_node_visible(
						    ui, bar->panels_button),
					    root->pointer.at))) {
				bar->menu.open = false;
			}
		}
		if (fired != VOE_EDITOR_CLOSABLE_COUNT) {
			voe_base_report_error_clear();
			if (!voe_editor_panels_toggle(fired, root, bar,
						      project_panel, preferences,
						      session, breakdown))
				voe_editor_notice_from_report(&session->notice,
							      "editor_settings");
		}
		// Whatever the browser or Preferences show: the bar is drawn
		// under both, and the next frame is laid out at this measure.
		voe_editor_topbar_measure(ui, bar);

		// THE BROWSER, WHEN IT WAS SHOWING, INSTEAD OF THE TOP BAR —
		// see the header on why `browsing` and not browser->showing.
		// A button that fired is carried out on `session`, which may
		// replace `scene->world` (a NEW, or an Open's Confirm, that
		// goes ahead) — after the reads above, which are this frame's
		// own world and this frame's own selection, and before
		// anything downstream reads either.
		if (browsing) {
			voe_editor_browser_result result =
				voe_editor_browser_clicks_read(ui, browser,
							       escape);
			voe_editor_session_browser_do(session, scene, browser,
						      result);
		} else {
			voe_editor_command clicked =
				voe_editor_topbar_clicks_read(ui, bar);

			if (clicked != VOE_EDITOR_COMMAND_NONE)
				voe_editor_session_do(session, scene, browser,
						      clicked);
			// One of the Project panel and Preferences at a time.
			if (voe_editor_topbar_project_read(ui, bar)) {
				voe_editor_preferences_hide(preferences);
				voe_editor_project_panel_show(project_panel);
			}
			if (voe_editor_topbar_preferences_read(ui, bar)) {
				voe_editor_project_panel_hide(project_panel);
				voe_editor_preferences_show(preferences);
			}
			// Panels flips its list as it was drawn this frame.
			if (voe_editor_topbar_panels_read(ui, bar))
				bar->menu.open = !menuing;
		}

		// THE PROJECT PANEL, WHEN IT WAS DRAWN: a changed window is
		// written at once (project.h), a failure said in the notice.
		if (projecting) {
			voe_editor_project_panel_result result =
				voe_editor_project_panel_clicks_read(
					ui, project_panel);

			// A false has already said why in the notice.
			if (result.changed)
				(void)voe_editor_project_window_set(
					session->project, result.window,
					&session->notice);
			if (result.closed)
				voe_editor_project_panel_hide(project_panel);
		}

		// THE ERRORS PANEL, WHEN IT WAS DRAWN: Close hides it.
		if (erroring &&
		    voe_editor_errors_clicks_read(ui, &session->errors))
			voe_editor_errors_hide(&session->errors);

		// THE FRAME BREAKDOWN, WHEN IT WAS DRAWN: its × hides it.
		if (framing &&
		    voe_editor_frame_breakdown_clicks_read(ui, breakdown))
			voe_editor_frame_breakdown_hide(breakdown);

		// PREFERENCES, WHEN IT WAS DRAWN. A theme chosen here is set on
		// the context after this frame's records were built, so it
		// restyles the next frame (ui/widgets.h's voe_ui_theme_set).
		// The two sliders move the theme in force as they are dragged —
		// its palette is derived again where it stands, so the next frame
		// draws with it and nothing is set here — and what they are left
		// at is remembered per theme when the drag ends (ADR-0197).
		if (preferring) {
			voe_editor_preferences_result result =
				voe_editor_preferences_clicks_read(ui,
								   preferences);

			if (result.action == VOE_EDITOR_PREFERENCES_CHOOSE) {
				const voe_editor_theme *chosen;

				if (!voe_editor_themes_choose(themes,
							      result.index))
					voe_editor_notice_set(
						&session->notice,
						"the chosen theme could not be remembered");
				chosen = voe_editor_themes_chosen(themes);
				voe_ui_font_set(ui, chosen->palette.font);
				voe_ui_theme_set(ui, &chosen->palette);
			} else if (result.action ==
				   VOE_EDITOR_PREFERENCES_ADJUST) {
				voe_editor_themes_adjust(themes, themes->chosen,
							 result.contrast,
							 result.separation,
							 result.text_scale);
			} else if (result.action ==
				   VOE_EDITOR_PREFERENCES_RESET) {
				voe_editor_themes_reset(themes, themes->chosen);
			} else if (result.action ==
				   VOE_EDITOR_PREFERENCES_CLOSE) {
				voe_editor_preferences_hide(preferences);
			}

			if (!result.sliding &&
			    !voe_editor_themes_scalars_write(themes))
				voe_editor_notice_set(
					&session->notice,
					"the slider values could not be remembered");
		}

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
