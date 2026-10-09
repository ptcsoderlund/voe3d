// The commands interface.c's one read carries out that are not the Assets
// panel's; see interface_read.h for when each is called and why it is apart.
//
// THE TOP BAR'S, THE BROWSER'S, PREFERENCES' AND THE ERRORS PANEL'S CLICKS
// ARE COMMANDS CARRIED OUT HERE, through voe_editor_session_do and
// voe_editor_session_browser_do, because the window between voe_ui_frame_end
// and the rewind is the only place topbar.h's and browser.h's recorded buttons
// can be asked. Only one of the two is ever read in a given frame — see
// voe_editor_interface_draw's own header on `browsing`. Preferences' Choose is
// carried out here too, through themes.h, and its palette set on the context
// for the next frame; the Project panel's window is written through project.h,
// the Landscape panel's size and cells through models.h.
//
// THE COLOUR PICKER'S RESULT IS READ HERE, for the same reason: it is a `ui`
// widget answering after voe_ui_frame_end. A `changed` goes through
// inspector.h's voe_editor_inspector_colour_submit at once, so the shape
// changes live, or, for the open material, into its shown copy; an `outside`
// press closes it. So is the open material's section: an edit the table's row
// takes, and the store told (models.h).
//
// THE SCENE LIST'S DROP IS CARRIED OUT HERE, through scene_list.h's
// voe_editor_scene_list_drop, after the rows' clicks. Released over the Assets
// leaf it makes a prefab (prefabs.h), one structural change; whether the
// Assets leaf takes it is !voe_editor_prefab_make_refused for the session's
// project, which the ghost shows as refused or not. Then scene.h's reveal
// shows a selection made elsewhere in the list.
//
// THE TOP BAR'S PANELS LIST (panels_menu.h) is read here: Panels flips it, a
// fired row goes through panels.h's voe_editor_panels_toggle and closes it, as
// does a press off the list and Panels. A fired panel header's × goes through
// the same toggle on its root.
#include "interface_read.h"

#include "errors.h"
#include "inspector.h"
#include "inspector_edit.h"
#include "inspector_material.h"
#include "inspector_place.h"
#include "inspector_sculpt.h"
#include "notice.h"
#include "panels.h"
#include "panels_menu.h"
#include "prefabs.h"
#include "project.h"
#include "scene_list.h"

#include <base/assert.h>
#include <base/report.h>

#include <string.h>

static void voe_editor_interface_picker_read(voe_ui_context *ui,
					     voe_editor_scene *scene,
					     voe_ui_node picker,
					     const voe_editor_picking *picked)
{
	voe_ui_colour_result result;

	VOE_BASE_ASSERT(ui != NULL && scene != NULL,
			"reading the picker with no frame or no scene");
	VOE_BASE_ASSERT(picked != NULL, "reading the picker for nothing");
	if (picker == VOE_UI_NODE_NONE)
		return;
	result = voe_ui_colour_picker_action(ui, picker);
	// The open material's colour is the shown copy's; its read below
	// carries it to the table's row and the store this same frame.
	if (result.changed && picked->material) {
		if (scene->material_open[0] != '\0') {
			scene->material.colour[0] = result.value.x;
			scene->material.colour[1] = result.value.y;
			scene->material.colour[2] = result.value.z;
		}
	} else if (result.changed)
		voe_editor_inspector_colour_submit(&scene->inspector,
						   scene->world, picked->entity,
						   picked->type, picked->offset,
						   result.value);
	if (result.outside)
		voe_editor_scene_picker_close(scene);
}

// The open material's section read into the shown copy; when that now differs
// from the table's row, the row takes it and the store is told whether a map
// changed (models.h). A row gone from the table takes nothing.
static void voe_editor_interface_material_read(const voe_ui_context *ui,
					       voe_editor_scene *scene,
					       voe_editor_models *models)
{
	voe_game_material *row;
	const voe_assets_material_file *shown = &scene->material;
	bool maps;

	VOE_BASE_ASSERT(ui != NULL && scene != NULL && models != NULL,
			"reading a material with no frame, scene or store");
	voe_editor_inspector_material_read(ui, scene);
	if (scene->material_open[0] == '\0')
		return;
	row = voe_editor_materials_find(voe_editor_models_materials(models),
					scene->material_open);
	if (row == NULL || memcmp(&row->values, shown, sizeof *shown) == 0)
		return;
	maps = strcmp(row->values.colormap, shown->colormap) != 0 ||
	       strcmp(row->values.normalmap, shown->normalmap) != 0 ||
	       strcmp(row->values.ormmap, shown->ormmap) != 0;
	row->values = *shown;
	voe_editor_models_material_changed(models, scene->material_open, maps);
	VOE_BASE_ASSERT(memcmp(&row->values, shown, sizeof *shown) == 0,
			"a row that did not take the shown material");
}

// The edits go first, and which order they are in is a fact and not a taste:
// a click read here can move the selection, and the controls were drawn for
// whatever was selected when the frame was built. The inspector keeps that
// entity itself, so the order is belt as well as braces. The colour picker is
// read before the Inspector's buttons, so a swatch that fires in the frame an
// outside press closed the picker opens it again rather than being closed
// behind.
bool voe_editor_interface_scene_read(voe_ui_context *ui, voe_base_arena *arena,
				     const voe_editor_dock_root *root,
				     voe_editor_scene *scene,
				     voe_editor_models *models,
				     voe_editor_session *session,
				     const voe_editor_topbar *bar,
				     voe_ui_node picker,
				     const voe_editor_picking *picked)
{
	// Why a Scene list drag over Assets would be refused, never shown:
	// the release's make says it in the session's notice.
	voe_editor_notice unshown = { 0 };
	bool over_assets;

	VOE_BASE_ASSERT(ui != NULL && arena != NULL && root != NULL,
			"reading the scene with no frame, scratch or root");
	VOE_BASE_ASSERT(scene != NULL && session != NULL && bar != NULL,
			"reading the scene with no scene, session or bar");
	voe_editor_inspector_edits_read(&scene->inspector, ui, scene->world);
	voe_editor_interface_picker_read(ui, scene, picker, picked);
	voe_editor_inspector_buttons_read(&scene->inspector, ui, scene,
					  root->pointer.down, root->pointer.at);
	voe_editor_inspector_sculpt_read(ui, &scene->sculpt);
	voe_editor_interface_material_read(ui, scene, models);
	if (!voe_editor_scene_clicks_read(scene, ui))
		voe_editor_notice_set(&session->notice, "The scene is full.");
	over_assets = voe_editor_dock_over_panel(
		root, voe_editor_topbar_high(bar, root->size.y),
		VOE_EDITOR_PANEL_ASSETS, root->pointer.at);
	voe_editor_scene_list_drop(
		scene, ui, root->pointer.down, root->pointer.at, over_assets,
		!voe_editor_prefab_make_refused(session->project,
						scene->list_held, &unshown));
	if (scene->list_made.generation != 0) {
		if (voe_editor_prefab_make(session->project, scene->list_made,
					   scene->assets.shown, arena,
					   &session->notice))
			scene->structural++;
		scene->list_made = (voe_ecs_entity){ 0 };
	}
	// After every read that can move the selection this frame.
	voe_editor_scene_reveal(scene, ui);
	return over_assets;
}

// A fired × closes its panel on this root, laid out so from the next frame,
// and remembers it as a drag's end does (main.c). A row of the Panels list
// fired is toggled the same way and closes the list; so does a press off the
// list and Panels.
static void voe_editor_interface_panels_read(
	voe_ui_context *ui, voe_editor_dock_root *root,
	const voe_editor_dock_closes *closes, voe_editor_session *session,
	voe_editor_topbar *bar, voe_editor_preferences *preferences,
	voe_editor_project_panel *project_panel,
	voe_editor_frame_breakdown *breakdown, bool menuing)
{
	voe_editor_closable fired = voe_editor_dock_closes_read(ui, closes);

	VOE_BASE_ASSERT(root != NULL && session != NULL && bar != NULL,
			"reading the panels with no root, session or bar");
	VOE_BASE_ASSERT(preferences != NULL && project_panel != NULL &&
				breakdown != NULL,
			"reading the panels with a panel missing");
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
				    voe_ui_node_visible(ui, bar->panels_button),
				    root->pointer.at))) {
			bar->menu.open = false;
		}
	}
	if (fired != VOE_EDITOR_CLOSABLE_COUNT) {
		voe_base_report_error_clear();
		if (!voe_editor_panels_toggle(fired, root, bar, project_panel,
					      preferences, session, breakdown))
			voe_editor_notice_from_report(&session->notice,
						      "editor_settings");
	}
}

// THE BROWSER, WHEN IT WAS SHOWING, INSTEAD OF THE TOP BAR — see interface.h
// on why `browsing` and not browser->showing. A button that fired is carried
// out on `session`, which may replace `scene->world` (a NEW, or an Open's
// Confirm, that goes ahead) — after the reads before this one, which are this
// frame's own world and this frame's own selection, and before anything
// downstream reads either.
static void voe_editor_interface_bar_read(
	voe_ui_context *ui, voe_editor_scene *scene, voe_editor_session *session,
	voe_editor_topbar *bar, voe_editor_browser *browser,
	voe_editor_preferences *preferences,
	voe_editor_project_panel *project_panel,
	voe_editor_landscape_panel *landscape_panel,
	voe_editor_interface_shown shown, bool escape)
{
	voe_editor_command clicked;

	VOE_BASE_ASSERT(ui != NULL && scene != NULL && session != NULL,
			"reading the bar with no frame, scene or session");
	VOE_BASE_ASSERT(bar != NULL && browser != NULL,
			"reading the bar with no bar or no browser");
	if (shown.browsing) {
		voe_editor_session_browser_do(
			session, scene, browser,
			voe_editor_browser_clicks_read(ui, browser, escape));
		return;
	}
	clicked = voe_editor_topbar_clicks_read(ui, bar);
	if (clicked != VOE_EDITOR_COMMAND_NONE)
		voe_editor_session_do(session, scene, browser, clicked);
	// One of the Project panel and Preferences at a time.
	if (voe_editor_topbar_project_read(ui, bar)) {
		voe_editor_preferences_hide(preferences);
		voe_editor_landscape_panel_hide(landscape_panel);
		voe_editor_project_panel_show(project_panel);
	}
	if (voe_editor_topbar_preferences_read(ui, bar)) {
		voe_editor_project_panel_hide(project_panel);
		voe_editor_landscape_panel_hide(landscape_panel);
		voe_editor_preferences_show(preferences);
	}
	// Panels flips its list as it was drawn this frame.
	if (voe_editor_topbar_panels_read(ui, bar))
		bar->menu.open = !shown.menuing;
}

// The Project, Landscape, Errors and frame breakdown panels, each read only
// when it was drawn.
static void voe_editor_interface_covers_read(
	voe_ui_context *ui, voe_base_arena *arena, voe_editor_models *models,
	voe_editor_session *session, voe_editor_project_panel *project_panel,
	voe_editor_landscape_panel *landscape_panel,
	voe_editor_frame_breakdown *breakdown, voe_editor_interface_shown shown)
{
	VOE_BASE_ASSERT(ui != NULL && arena != NULL && session != NULL,
			"reading the covers with no frame, scratch or session");
	VOE_BASE_ASSERT(project_panel != NULL && landscape_panel != NULL &&
				breakdown != NULL,
			"reading the covers with a panel missing");
	// A changed window is written at once (project.h), a failure said in
	// the notice.
	if (shown.projecting) {
		voe_editor_project_panel_result result =
			voe_editor_project_panel_clicks_read(ui, project_panel);

		// A false has already said why in the notice.
		if (result.changed)
			(void)voe_editor_project_window_set(
				session->project, result.window,
				&session->notice);
		if (result.closed)
			voe_editor_project_panel_hide(project_panel);
	}
	// A new shape is written at once (models.h) and shown, a refusal said
	// in the notice.
	if (shown.landscaping) {
		voe_editor_landscape_panel_result result =
			voe_editor_landscape_panel_clicks_read(ui,
							       landscape_panel);

		if (result.changed && session->project->folder != NULL &&
		    voe_editor_models_landscape_shape(
			    models, session->project->folder,
			    landscape_panel->path, result.size, result.cells,
			    arena, &session->notice))
			(void)voe_editor_landscape_panel_show(
				landscape_panel, landscape_panel->path,
				result.size, result.cells);
		if (result.closed)
			voe_editor_landscape_panel_hide(landscape_panel);
	}
	// The Errors panel's Close hides it; the frame breakdown's × hides it.
	if (shown.erroring &&
	    voe_editor_errors_clicks_read(ui, &session->errors))
		voe_editor_errors_hide(&session->errors);
	if (shown.framing &&
	    voe_editor_frame_breakdown_clicks_read(ui, breakdown))
		voe_editor_frame_breakdown_hide(breakdown);
}

// A theme chosen here is set on the context after this frame's records were
// built, so it restyles the next frame (ui/widgets.h's voe_ui_theme_set). The
// two sliders move the theme in force as they are dragged — its palette is
// derived again where it stands, so the next frame draws with it and nothing
// is set here — and what they are left at is remembered per theme when the
// drag ends (ADR-0197).
static void voe_editor_interface_preferences_read(
	voe_ui_context *ui, voe_editor_session *session,
	voe_editor_preferences *preferences, voe_editor_themes *themes)
{
	voe_editor_preferences_result result;

	VOE_BASE_ASSERT(ui != NULL && session != NULL,
			"reading Preferences with no frame or no session");
	VOE_BASE_ASSERT(preferences != NULL && themes != NULL,
			"reading Preferences with no panel or no themes");
	result = voe_editor_preferences_clicks_read(ui, preferences);
	if (result.action == VOE_EDITOR_PREFERENCES_CHOOSE) {
		const voe_editor_theme *chosen;

		if (!voe_editor_themes_choose(themes, result.index))
			voe_editor_notice_set(
				&session->notice,
				"the chosen theme could not be remembered");
		chosen = voe_editor_themes_chosen(themes);
		voe_ui_font_set(ui, chosen->palette.font);
		voe_ui_theme_set(ui, &chosen->palette);
	} else if (result.action == VOE_EDITOR_PREFERENCES_ADJUST) {
		voe_editor_themes_adjust(themes, themes->chosen,
					 result.contrast, result.separation,
					 result.text_scale);
	} else if (result.action == VOE_EDITOR_PREFERENCES_RESET) {
		voe_editor_themes_reset(themes, themes->chosen);
	} else if (result.action == VOE_EDITOR_PREFERENCES_CLOSE) {
		voe_editor_preferences_hide(preferences);
	}

	if (!result.sliding && !voe_editor_themes_scalars_write(themes))
		voe_editor_notice_set(
			&session->notice,
			"the slider values could not be remembered");
}

void voe_editor_interface_commands_read(
	voe_ui_context *ui, voe_base_arena *arena, voe_editor_dock_root *root,
	const voe_editor_dock_closes *closes, voe_editor_scene *scene,
	voe_editor_models *models, voe_editor_session *session,
	voe_editor_topbar *bar, voe_editor_browser *browser,
	voe_editor_preferences *preferences,
	voe_editor_project_panel *project_panel,
	voe_editor_landscape_panel *landscape_panel, voe_editor_themes *themes,
	voe_editor_frame_breakdown *breakdown, voe_editor_interface_shown shown,
	bool escape)
{
	VOE_BASE_ASSERT(ui != NULL && arena != NULL && root != NULL,
			"reading commands with no frame, scratch or root");
	VOE_BASE_ASSERT(closes != NULL && themes != NULL,
			"reading commands with no closes or no themes");
	voe_editor_interface_panels_read(ui, root, closes, session, bar,
					 preferences, project_panel, breakdown,
					 shown.menuing);
	// Whatever the browser or Preferences show: the bar is drawn under
	// both, and the next frame is laid out at this measure.
	voe_editor_topbar_measure(ui, bar);
	voe_editor_interface_bar_read(ui, scene, session, bar, browser,
				      preferences, project_panel,
				      landscape_panel, shown, escape);
	voe_editor_interface_covers_read(ui, arena, models, session,
					 project_panel, landscape_panel,
					 breakdown, shown);
	if (shown.preferring)
		voe_editor_interface_preferences_read(ui, session, preferences,
						      themes);
}
