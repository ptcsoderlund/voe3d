// The three per-frame command stretches of main.c's loop, in the order main.c
// calls them: the history's step (an open material's edit written and pushed
// at rest first, a different project closing the open material), the
// keyboard's read and acts (F2's rename and an asset's Delete question among
// them, the Assets panel's keyboard a guard, `ui`'s keyboard fed, Escape
// closing that question first, choosing no brush after the lists and hiding
// the Landscape panel with the Project panel), and the acts that wait for the
// interface to have drawn: Delete, Ctrl+D, R, the edit, and a reveal's unfold
// marked. See frame_commands.h.
#include "frame_commands.h"

#include "errors.h"
#include "notice.h"
#include "scene_list.h"

#include <base/assert.h>

#include <string.h>

// AN EDIT TO THE OPEN MATERIAL SETTLES AT REST (0399 point 9): its file is
// written, a refusal said in the notice, and one undo step pushed carrying
// the values before and after. Written at once, never by Save, so the project
// is not marked unsaved. Before a step is taken, so a Ctrl+Z read at rest
// goes back over this edit and not past it.
static void voe_editor_frame_commands_material(
	voe_editor_frame_commands *commands)
{
	voe_editor_session *session = commands->session;
	voe_editor_scene *scene = commands->scene;
	voe_base_error error = VOE_BASE_OK;

	VOE_BASE_ASSERT(scene != NULL && commands->scratch != NULL,
			"settling a material with no scene or scratch");
	if (!commands->at_rest || session->replaced ||
	    session->project->folder == NULL ||
	    scene->material_open[0] == '\0' ||
	    memcmp(&scene->material, &scene->material_before,
		   sizeof scene->material) == 0)
		return;
	if (!voe_editor_material_step_write(session->project->folder,
					    scene->material_open,
					    &scene->material, commands->scratch,
					    &error))
		voe_editor_notice_set(&session->notice, "Could not save %s: %s",
				      scene->material_open,
				      voe_base_error_string(error));
	voe_editor_undo_material(commands->undo, session->project,
				 commands->scratch,
				 voe_editor_material_step_new(
					 scene->material_open,
					 &scene->material_before,
					 &scene->material));
	scene->material_before = scene->material;
	VOE_BASE_ASSERT(memcmp(&scene->material, &scene->material_before,
			       sizeof scene->material) == 0,
			"a settled material still differs from its file");
}

void voe_editor_frame_commands_history(voe_editor_frame_commands *commands)
{
	VOE_BASE_ASSERT(commands != NULL, "a history step for no commands");
	VOE_BASE_ASSERT(commands->session != NULL, "a history step in no session");

	voe_editor_session *session = commands->session;

	voe_editor_frame_commands_material(commands);

	// A DIFFERENT PROJECT EMPTIES THE HISTORY, AND OTHERWISE LAST
	// FRAME'S CTRL+Z OR CTRL+Y IS TAKEN HERE — before world_step.h,
	// so the rows a step puts back are given their meshes before
	// anything draws them (undo.h). A step leaves the project
	// unsaved and is never itself an edit to record.
	// A prefab opened sets the level's line aside, and Back puts it
	// back (0283 point 8).
	// The Landscape panel's path is the old project's, so it closes too,
	// and so does the open material: the new table may hold its path.
	if (session->replaced) {
		session->replaced = false;
		voe_editor_landscape_panel_hide(commands->landscape_panel);
		commands->scene->material_open[0] = '\0';
		voe_editor_undo_forget(commands->undo);
		voe_editor_views_focus_camera(commands->views,
					      commands->scene->world);
	} else if (session->prefab_opened) {
		session->prefab_opened = false;
		voe_editor_undo_aside(commands->undo);
		voe_editor_views_focus_camera(commands->views,
					      commands->scene->world);
	} else if (session->prefab_closed) {
		session->prefab_closed = false;
		voe_editor_undo_restore(commands->undo);
		voe_editor_views_focus_camera(commands->views,
					      commands->scene->world);
	} else if ((commands->step_back || commands->step_forward) &&
		   voe_editor_undo_take(commands->undo, session->project,
					commands->scene, commands->models,
					commands->scratch, &session->notice,
					commands->step_forward)) {
		voe_editor_session_edited(session);
	}
	commands->step_back = false;
	commands->step_forward = false;

	VOE_BASE_ASSERT(!commands->step_back && !commands->step_forward,
			"a history step left an edge standing");
}

voe_ui_keyboard
voe_editor_frame_commands_read(voe_editor_frame_commands *commands,
			       const voe_editor_keys_frame *keyboard,
			       const voe_platform_text *text, bool left,
			       bool flying)
{
	VOE_BASE_ASSERT(commands != NULL, "a keyboard read for no commands");
	VOE_BASE_ASSERT(keyboard != NULL && text != NULL,
			"a keyboard read of no keyboard");

	voe_editor_session *session = commands->session;
	voe_editor_scene *scene = commands->scene;
	voe_editor_browser *browser = commands->browser;
	voe_editor_shortcuts shortcuts;

	// WHICH EDGE MEANT WHICH COMMAND IS ANSWERED ONCE, HERE
	// (shortcuts.h), out of the keyboard and the guards main.c
	// is the one holding; everything below it is acting on a flag.
	shortcuts = voe_editor_shortcuts_read(
		keyboard, (voe_editor_shortcuts_guards){
				  .browser_showing = browser->showing,
				  .typing = voe_ui_typing(commands->ui),
				  .picker_open = scene->picking.open,
				  .dropdown_open = scene->dropdown.open,
				  .pointer_down = left,
				  .flying = flying,
				  .assets_keyboard = scene->assets.keyboard });
	commands->shortcuts = shortcuts;

	// F2 renames the Assets panel's selected row while the panel holds
	// the keyboard; its field is drawn from this frame (assets_panel.h).
	if (shortcuts.rename && scene->assets.keyboard)
		voe_editor_assets_rename_begin(&scene->assets);
	// Delete asks about it first; interface.c opens the question.
	if (shortcuts.delete_asset)
		voe_editor_assets_delete_begin(&scene->assets);

	// Ctrl+N, Ctrl+O and Ctrl+S are the bar's three commands.
	if (shortcuts.new_project)
		voe_editor_session_do(session, scene, browser,
				      VOE_EDITOR_COMMAND_NEW);
	if (shortcuts.open)
		voe_editor_session_do(session, scene, browser,
				      VOE_EDITOR_COMMAND_OPEN);
	if (shortcuts.save)
		voe_editor_session_do(session, scene, browser,
				      VOE_EDITOR_COMMAND_SAVE);

	// Delete and Ctrl+D act after the interface has drawn, because
	// the dock walk zeroes the scene's `structural` and `full` for
	// the frame (scene.h); Ctrl+Z and Ctrl+Y at the top of the next
	// frame, with the rest they were read at beside them.
	commands->at_rest = shortcuts.at_rest;
	commands->step_back = shortcuts.undo;
	commands->step_forward = shortcuts.redo;

	// ESCAPE'S ORDER IS THIS FILE'S, out of the free edge that read
	// leaves: an open Delete question is cancelled first, then a Scene
	// list drag under way, then the picker closes and goes no further,
	// otherwise it is the browser's Cancel or Preferences' Close.
	commands->escape_free = shortcuts.escape_free;
	if (commands->escape_free && session->asking.open) {
		voe_editor_assets_ask_close(&session->asking);
		commands->escape_free = false;
	}
	if (commands->escape_free && voe_editor_scene_list_cancel(scene))
		commands->escape_free = false;
	if (commands->escape_free && scene->picking.open) {
		voe_editor_scene_picker_close(scene);
		commands->escape_free = false;
	}
	// A chosen brush is put down once no Inspector list or Assets menu is
	// open to take the edge first (interface.c closes those on it).
	if (commands->escape_free && scene->sculpt.chosen &&
	    !scene->dropdown.open && !scene->inspector.adding &&
	    !scene->assets.menu.open) {
		voe_editor_sculpt_choose_none(&scene->sculpt);
		commands->escape_free = false;
	}
	// THE BROWSER KEEPS ESCAPE WHILE IT SHOWS; otherwise it hides
	// Preferences and the Errors panel, which it does nothing else
	// to. Neither while a person is typing: then it cancels that
	// and nothing more.
	if (commands->escape_free && !browser->showing) {
		voe_editor_preferences_hide(commands->preferences);
		voe_editor_project_panel_hide(commands->project_panel);
		voe_editor_landscape_panel_hide(commands->landscape_panel);
		voe_editor_errors_hide(&session->errors);
	}

	// BESIDE THE POINTER, AND FOR THE SAME REASON (dock.h): `ui`
	// reads this for whichever field or number box is focused, and
	// nothing here decides which one that is.
	// A flying view keeps the keys it reads and the pointer: `ui`
	// gets no text, no Backspace, Enter or Tab, and no pointer.
	return (voe_ui_keyboard){
		.text = flying ? NULL : text->bytes,
		.size = flying ? 0 : text->size,
		.backspace = !flying &&
			     keyboard->pressed[VOE_PLATFORM_KEY_BACKSPACE],
		.enter = !flying && keyboard->pressed[VOE_PLATFORM_KEY_ENTER],
		.escape = shortcuts.escape,
		.tab = !flying && keyboard->pressed[VOE_PLATFORM_KEY_TAB],
	};
}

void voe_editor_frame_commands_after_draw(voe_editor_frame_commands *commands)
{
	VOE_BASE_ASSERT(commands != NULL, "acts after the draw for no commands");
	VOE_BASE_ASSERT(commands->scene != NULL && commands->gizmo != NULL,
			"acts after the draw on no scene");

	voe_editor_scene *scene = commands->scene;

	// Only when the Inspector's own buttons changed nothing
	// structural this frame: two changes before the queue is
	// applied would be given one id (entities.h).
	if (scene->structural == 0 && commands->shortcuts.delete_entity)
		voe_editor_scene_delete(scene);
	if (scene->structural == 0 && commands->shortcuts.duplicate)
		voe_editor_scene_duplicate(scene);
	if (commands->shortcuts.gizmo_switch)
		voe_editor_scene_gizmo_switch(scene);
	if (scene->full)
		voe_editor_notice_set(&commands->session->notice,
				      "The scene is full.");
	// AN EDIT REACHED THE PROJECT, AND NOTHING ABOVE ASKED
	// FOR IT AS A COMMAND — an Inspector number dragged, an
	// entity Add entity, Delete or Duplicate queued, a gizmo
	// move: the other half of what marks the project
	// unsaved (session.h, scene.h, gizmo.h).
	if (scene->inspector.replaced > 0 || scene->structural > 0 ||
	    commands->gizmo->moved > 0) {
		voe_editor_session_edited(commands->session);
		voe_editor_undo_edited(commands->undo);
	}
	// A reveal's unfold reached the project: unsaved, but it amends the
	// undo state instead of being a step (undo.h).
	if (scene->unfolded > 0) {
		voe_editor_session_edited(commands->session);
		voe_editor_undo_revealed(commands->undo);
	}
}
