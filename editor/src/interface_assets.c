// The Assets panel's requests carried out inside interface.c's one read; see
// interface_assets.h for when it is called and why it is a file of its own.
//
// THE ASSETS PANEL'S REQUESTS ARE CARRIED OUT HERE: a fired prefab row opens
// (session.h), beside where Import shows the browser; a naming request (a
// rename, folder, landscape or material) goes through assets_manage.h in the frame's
// arena, a rename's typed `/` refused here, and the Landscape panel opens on a
// landscape made or fired; a fired material row opens in the Inspector (scene.h).
// Delete asks first: the request opens the session's question (assets_ask.h),
// drawn over the dock by interface.c and read here — Delete trashes through
// assets_manage.h, and it or Cancel or a press outside closes it. The open
// material follows each successful rename and trash.
//
// THE ASSETS PANEL'S RIGHT-BUTTON MENU (assets_menu.h) is drawn by interface.c
// after the panels list and read here after the panel: Rename, Delete, Folder,
// Landscape and Material begin on the panel, Duplicate goes through assets_manage.h.
// Escape closing it first, and the dock's covers, are interface.c's.
#include "interface_assets.h"

#include "assets_ask.h"
#include "assets_manage.h"
#include "assets_menu.h"
#include "notice.h"

#include <base/assert.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define LANDSCAPE_ENDING ".landscape"

// The file Create -> Landscape made of `folder` and `name`, as
// assets_manage.c names it — `.landscape` appended unless `name` ends so in
// any case — written `Assets/...` into `out` of VOE_SCENE_PREFAB_PATH bytes,
// "" when it does not fit.
static void voe_editor_interface_assets_made_path(char *out, const char *folder,
						  const char *name)
{
	size_t length = strlen(name);
	size_t tail = sizeof LANDSCAPE_ENDING - 1;
	bool ends = length >= tail;
	int written;

	VOE_BASE_ASSERT(out != NULL && folder != NULL,
			"a landscape's path into nowhere or from no folder");
	for (size_t i = 0; ends && i < tail; i++)
		ends = tolower((unsigned char)name[length - tail + i]) ==
		       LANDSCAPE_ENDING[i];
	written = snprintf(out, VOE_SCENE_PREFAB_PATH, "Assets/%s%s%s%s",
			   folder, folder[0] != '\0' ? "/" : "", name,
			   ends ? "" : LANDSCAPE_ENDING);
	if (written < 0 || written >= VOE_SCENE_PREFAB_PATH)
		out[0] = '\0';
}

// The Assets panel's request carried out and cleared. A rename's `to` is the
// shown folder joined with the typed name, so a `/` in it would move the file
// into a folder rather than rename it (assets_manage.h's constraint); it is
// refused here in the words assets_manage.c uses for the other separators.
// A landscape made leaves its `Assets/...` path in `made`, of
// VOE_SCENE_PREFAB_PATH bytes, "" otherwise or when it does not fit.
static void voe_editor_interface_assets_request_do(
	voe_editor_session *session, voe_editor_scene *scene,
	voe_editor_undo *undo, voe_editor_models *models,
	voe_base_arena *arena, char *made)
{
	voe_editor_assets_request *request = &scene->assets.request;

	VOE_BASE_ASSERT(session != NULL && scene != NULL && undo != NULL,
			"an Assets request with no session, scene or undo");
	VOE_BASE_ASSERT(arena != NULL && made != NULL,
			"an Assets request with no scratch or no made path");
	made[0] = '\0';
	if (request->kind == VOE_EDITOR_ASSETS_NAMING_RENAME) {
		if (strchr(request->name, '/') != NULL)
			voe_editor_notice_set(&session->notice,
					      "%s: a name cannot hold /, \\ or \"",
					      request->name);
		// A false move has said why in the notice.
		else if (voe_editor_assets_move(session, scene, undo, models,
						arena, request->from,
						request->to))
			voe_editor_scene_material_follow(scene, request->from,
							 request->to);
	} else if (request->kind == VOE_EDITOR_ASSETS_NAMING_FOLDER) {
		(void)voe_editor_assets_folder_make(session, scene, undo, arena,
						    request->folder,
						    request->name);
	} else if (request->kind == VOE_EDITOR_ASSETS_NAMING_LANDSCAPE &&
		   voe_editor_assets_landscape_make(session, arena,
						    request->folder,
						    request->name)) {
		voe_editor_assets_list_due(&scene->assets);
		voe_editor_interface_assets_made_path(made, request->folder,
						      request->name);
	} else if (request->kind == VOE_EDITOR_ASSETS_NAMING_MATERIAL &&
		   voe_editor_assets_material_make(session, models, arena,
						   request->folder,
						   request->name)) {
		voe_editor_assets_list_due(&scene->assets);
	}
	request->kind = VOE_EDITOR_ASSETS_NAMING_NONE;
}

// Opens the Landscape panel on `path` (`Assets/...`) at the size and cells its
// store entry or file holds, hiding Preferences and the Project panel in its place.
// A file that will not read is said in the notice and opens nothing.
static void voe_editor_interface_assets_landscape_open(
	voe_editor_session *session, voe_editor_models *models,
	voe_base_arena *arena, voe_editor_landscape_panel *panel,
	voe_editor_preferences *preferences,
	voe_editor_project_panel *project_panel, const char *path)
{
	float size;
	uint32_t cells;

	VOE_BASE_ASSERT(session != NULL && models != NULL && arena != NULL,
			"opening a landscape with no session, store or scratch");
	VOE_BASE_ASSERT(panel != NULL && path != NULL,
			"opening no landscape panel or no path");
	if (session->project->folder == NULL ||
	    !voe_editor_models_landscape_found(models, session->project->folder,
					       path, arena, &size, &cells,
					       &session->notice))
		return;
	if (!voe_editor_landscape_panel_show(panel, path, size, cells))
		return;
	voe_editor_preferences_hide(preferences);
	voe_editor_project_panel_hide(project_panel);
}

// Opens `path` (`Assets/...`) in the Inspector: the selection cleared, the path
// kept and the table's row copied as the shown material. A path the table
// lacks (a file that would not parse) opens nothing.
static void voe_editor_interface_assets_material_open(voe_editor_scene *scene,
						      voe_editor_models *models,
						      const char *path)
{
	const voe_game_material *row;

	VOE_BASE_ASSERT(scene != NULL && models != NULL,
			"opening a material in no scene or from no store");
	VOE_BASE_ASSERT(path != NULL, "opening no material");
	row = voe_editor_materials_find(voe_editor_models_materials(models),
					path);
	if (row == NULL ||
	    strlen(path) >= sizeof scene->material_open)
		return;
	voe_editor_scene_select(scene, (voe_ecs_entity){ 0 });
	snprintf(scene->material_open, sizeof scene->material_open, "%s",
		 path);
	scene->material = row->values;
}

// A fired row of the Assets menu carried out on the selected row or the shown
// folder (assets_panel.h, assets_manage.h).
static void voe_editor_interface_assets_menu_do(voe_editor_session *session,
						voe_editor_scene *scene,
						voe_editor_undo *undo,
						voe_editor_models *models,
						voe_base_arena *arena,
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
	else if (item == VOE_EDITOR_ASSETS_MENU_LANDSCAPE)
		voe_editor_assets_landscape_begin(&scene->assets);
	else if (item == VOE_EDITOR_ASSETS_MENU_MATERIAL)
		voe_editor_assets_material_begin(&scene->assets);
	else if (item == VOE_EDITOR_ASSETS_MENU_DUPLICATE &&
		 voe_editor_assets_selected_path(&scene->assets, path,
						 sizeof path))
		// A false has said why in the notice.
		(void)voe_editor_assets_duplicate(session, scene, undo, models,
						  arena, path);
}

void voe_editor_interface_assets_read(
	voe_ui_context *ui, voe_base_arena *arena,
	const voe_editor_dock_root *root, voe_editor_scene *scene,
	voe_editor_undo *undo, voe_editor_models *models,
	voe_editor_session *session, voe_editor_browser *browser,
	voe_editor_preferences *preferences,
	voe_editor_project_panel *project_panel,
	voe_editor_landscape_panel *landscape_panel, bool over_assets,
	bool menuing, bool asking)
{
	// A landscape Create made this frame, `Assets/...`.
	char made[VOE_SCENE_PREFAB_PATH];

	VOE_BASE_ASSERT(ui != NULL && arena != NULL && root != NULL,
			"reading the Assets panel with no frame, scratch or root");
	VOE_BASE_ASSERT(scene != NULL && session != NULL && browser != NULL,
			"reading the Assets panel with no scene, session or browser");
	if (voe_editor_assets_clicks_read(ui, &scene->assets,
					  root->pointer.down, over_assets))
		voe_editor_browser_show(browser, VOE_EDITOR_BROWSER_IMPORT,
					NULL, &session->notice);
	voe_editor_interface_assets_request_do(session, scene, undo, models,
					       arena, made);
	// A landscape made or a landscape row fired opens its panel.
	if (made[0] != '\0')
		voe_editor_interface_assets_landscape_open(
			session, models, arena, landscape_panel, preferences,
			project_panel, made);
	if (scene->assets.landscape_opened[0] != '\0') {
		voe_editor_interface_assets_landscape_open(
			session, models, arena, landscape_panel, preferences,
			project_panel, scene->assets.landscape_opened);
		scene->assets.landscape_opened[0] = '\0';
	}
	// After the panel's read, so a naming begun here is not ended by
	// a field it has not drawn yet, and before the question opens
	// from `deleting`.
	if (menuing)
		voe_editor_interface_assets_menu_do(
			session, scene, undo, models, arena,
			voe_editor_assets_menu_read(ui, &scene->assets.menu,
						    root->pointer.at,
						    root->pointer.down,
						    root->size));
	// The question as it was drawn answered before a new one opens,
	// so a question opened this frame is not read against nodes it
	// never had.
	if (asking) {
		voe_editor_assets_ask_answer answer =
			voe_editor_assets_ask_read(ui, &session->asking,
						   root->pointer.down,
						   root->pointer.at);

		// A false has said why in the notice.
		if (answer == VOE_EDITOR_ASSETS_ASK_DELETE &&
		    voe_editor_assets_trash(session, scene, undo, models, arena,
					    session->asking.path))
			voe_editor_scene_material_follow(
				scene, session->asking.path, NULL);
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
	if (scene->assets.material_opened[0] != '\0') {
		voe_editor_interface_assets_material_open(
			scene, models, scene->assets.material_opened);
		scene->assets.material_opened[0] = '\0';
	}
	if (scene->assets.opened[0] != '\0') {
		voe_editor_session_prefab_open(session, scene,
					       scene->assets.opened);
		scene->assets.opened[0] = '\0';
	}
}
