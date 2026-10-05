// One `ui` frame per root, submitted into the open frame and drawn with
// voe_render_element_transform. See the header for why a root is a frame and why
// the pointer arrives already in millimetres.
//
// NOTHING IN HERE DECIDES WHAT IS ON A PANEL. It opens the frame, hands over the
// pointer it was given, asks the dock to walk, and moves records into the
// device; what a panel says is dock.c's `voe_editor_panel_draw`.
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
// session's project, which the ghost shows as refused or not.
//
// A PREFAB ROW FIRED IN THE ASSETS PANEL OPENS IT, through
// voe_editor_session_prefab_open, beside where Import shows the browser; the
// bar then names the prefab's file and draws Back, whose command reaches
// voe_editor_session_do as the bar's others do.
//
// THE OPEN DROPDOWN'S LIST IS THE INSPECTOR'S OWN (inspector.h) AND NOT THIS
// FILE'S. It is drawn inside that panel so that it moves and disappears with the
// button it hangs from (ADR-0199), and the only thing this file still does to it
// is close it on Escape.
#include "interface.h"

#include "browser.h"
#include "errors.h"
#include "inspector.h"
#include "inspector_edit.h"
#include "notice.h"
#include "panels.h"
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

bool voe_editor_interface_draw(voe_render_device *gpu, voe_ui_context *ui,
			       voe_base_arena *arena,
			       voe_editor_dock_root *roots,
			       uint32_t count, voe_editor_scene *scene,
			       const voe_editor_assets_drag *drag,
			       voe_editor_views *views,
			       voe_editor_session *session,
			       voe_editor_topbar *bar,
			       voe_editor_browser *browser,
			       voe_editor_preferences *preferences,
			       voe_editor_project_panel *project_panel,
			       voe_editor_themes *themes, bool escape)
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
	VOE_BASE_ASSERT(session != NULL, "drawing an interface with no session");
	VOE_BASE_ASSERT(bar != NULL, "drawing an interface with no top bar");
	VOE_BASE_ASSERT(browser != NULL, "drawing an interface with no browser");
	VOE_BASE_ASSERT(preferences != NULL,
			"drawing an interface with no preferences");
	VOE_BASE_ASSERT(project_panel != NULL,
			"drawing an interface with no project panel");
	VOE_BASE_ASSERT(themes != NULL, "drawing an interface with no themes");

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
		// The picker, when it shows, and what it edits: the target as
		// it was when drawn, whatever this frame's clicks do to it.
		voe_math_float3 colour;
		voe_editor_picking picked;
		voe_ui_node picker = VOE_UI_NODE_NONE;
		bool picking;
		// Why a Scene list drag over Assets would be refused, never
		// shown: the release's make says it in the session's notice.
		voe_editor_notice unshown = { 0 };

		if (browsing || preferring || erroring || projecting)
			voe_editor_scene_picker_close(scene);
		picking = voe_editor_scene_picker_showing(scene, &colour);
		picked = scene->picking;
		// The open list and Add component's are the Inspector's own
		// (inspector.h); closing them on Escape is all this file does.
		if (escape) {
			voe_editor_scene_dropdown_close(scene);
			voe_editor_inspector_add_close(&scene->inspector);
		}

		below_bar.size.y -= voe_editor_topbar_high(bar, root->size.y);
		if (browsing || preferring || erroring || projecting)
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
		voe_editor_scene_list_drop(
			scene, ui, root->pointer.down, root->pointer.at,
			voe_editor_dock_over_panel(
				root, voe_editor_topbar_high(bar, root->size.y),
				VOE_EDITOR_PANEL_ASSETS, root->pointer.at),
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
		if (voe_editor_assets_clicks_read(ui, &scene->assets))
			voe_editor_browser_show(browser,
						VOE_EDITOR_BROWSER_IMPORT, NULL,
						&session->notice);
		if (scene->assets.opened[0] != '\0') {
			voe_editor_session_prefab_open(session, scene,
						       scene->assets.opened);
			scene->assets.opened[0] = '\0';
		}
		voe_editor_views_rects_read(views, ui);
		// A fired × closes its panel on this root, laid out so from the
		// next frame, and remembers it as a drag's end does (main.c).
		fired = voe_editor_dock_closes_read(ui, &closes);
		if (fired != VOE_EDITOR_CLOSABLE_COUNT) {
			voe_base_report_error_clear();
			if (!voe_editor_panels_toggle(fired, root, bar))
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
