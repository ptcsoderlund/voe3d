// The refuse-once rule, the five commands, and what a browser action does to
// the session. See the header for what each one does and why only CLOSE ever
// answers true.
#include "session.h"

#include "last_project.h"

#include <base/assert.h>

#include <ecs/world.h>

void voe_editor_session_edited(voe_editor_session *session)
{
	VOE_BASE_ASSERT(session != NULL, "marking no session edited");
	VOE_BASE_ASSERT(session->project != NULL,
			"marking a session with no project edited");

	session->project->unsaved = true;
	voe_editor_notice_clear(&session->notice);
	session->armed = VOE_EDITOR_COMMAND_NONE;
}

bool voe_editor_session_do(voe_editor_session *session, voe_editor_scene *scene,
			   voe_editor_browser *browser,
			   voe_editor_command command)
{
	// This frame's command is the same one refused last time, so it goes
	// ahead instead of being refused again — read before `armed` is
	// consumed below, which is the only place this matters.
	bool repeat;

	VOE_BASE_ASSERT(session != NULL, "doing a command on no session");
	VOE_BASE_ASSERT(session->project != NULL,
			"doing a command on a session with no project");
	VOE_BASE_ASSERT(scene != NULL, "doing a command with no scene");
	VOE_BASE_ASSERT(browser != NULL, "doing a command with no browser");
	VOE_BASE_ASSERT(command != VOE_EDITOR_COMMAND_NONE,
			"doing no command");

	repeat = command == session->armed;

	// EVERY CALL STARTS BY CLEARING THE NOTICE AND DISARMING. A repeat
	// that goes ahead has nothing left to explain — the project a NEW or
	// an Open just replaced, or the window a CLOSE is about to leave, is
	// not the one the notice was warning about — and a command that was
	// never armed had no notice of its own to keep either. Only a refusal
	// below puts one back, and only for the one command it just refused.
	voe_editor_notice_clear(&session->notice);
	session->armed = VOE_EDITOR_COMMAND_NONE;

	switch (command) {
	case VOE_EDITOR_COMMAND_CLOSE:
		if (session->project->unsaved && !repeat) {
			voe_editor_notice_set(
				&session->notice,
				"There are unsaved changes — close again to discard them");
			session->armed = VOE_EDITOR_COMMAND_CLOSE;
			return false;
		}
		voe_editor_play_end(&session->play);
		return true;

	case VOE_EDITOR_COMMAND_PLAY:
		// Never arms: Play takes the world as it is, unsaved or not,
		// and a second press is Stop.
		if (session->play.stage == VOE_EDITOR_PLAY_IDLE)
			voe_editor_play_start(&session->play, session->project,
					      &session->notice);
		else
			voe_editor_play_end(&session->play);
		return false;

	case VOE_EDITOR_COMMAND_NEW: {
		voe_editor_project *fresh;

		if (session->project->unsaved && !repeat) {
			voe_editor_notice_set(
				&session->notice,
				"There are unsaved changes — New again to discard them");
			session->armed = VOE_EDITOR_COMMAND_NEW;
			return false;
		}

		fresh = voe_editor_project_new_untitled();
		voe_editor_project_destroy(session->project);
		session->project = fresh;
		session->replaced = true;
		scene->world = fresh->world;
		scene->selected = (voe_ecs_entity){ 0 };
		return false;
	}

	case VOE_EDITOR_COMMAND_OPEN:
		if (session->project->unsaved && !repeat) {
			voe_editor_notice_set(
				&session->notice,
				"There are unsaved changes — Open again to discard them");
			session->armed = VOE_EDITOR_COMMAND_OPEN;
			return false;
		}
		// Allowed, once past the refusal above. What a chosen folder
		// does to the project is voe_editor_session_browser_do's, on
		// the browser's own Confirm.
		voe_editor_browser_show(browser, VOE_EDITOR_BROWSER_OPEN,
					&session->notice);
		return false;

	case VOE_EDITOR_COMMAND_SAVE:
		// An opened project is written back over itself; an untitled
		// one has nowhere to write to yet, so the browser opens in
		// SAVE mode instead and gives it one — what its Confirm does
		// is voe_editor_session_browser_do's.
		if (session->project->folder != NULL)
			(void)voe_editor_project_save(session->project, NULL,
						      &session->notice);
		else
			voe_editor_browser_show(browser,
						VOE_EDITOR_BROWSER_SAVE,
						&session->notice);
		return false;

	case VOE_EDITOR_COMMAND_NONE:
		break;
	}

	return false;
}

void voe_editor_session_browser_do(voe_editor_session *session,
				   voe_editor_scene *scene,
				   voe_editor_browser *browser,
				   voe_editor_browser_result result)
{
	VOE_BASE_ASSERT(session != NULL, "doing a browser action on no session");
	VOE_BASE_ASSERT(session->project != NULL,
			"doing a browser action on a session with no project");
	VOE_BASE_ASSERT(scene != NULL, "doing a browser action with no scene");
	VOE_BASE_ASSERT(browser != NULL, "doing no browser's action");

	// Nothing fired this frame: nothing here to clear a notice or disarm
	// over — see the header on why this is not the same as every other
	// call clearing unconditionally.
	if (result.action == VOE_EDITOR_BROWSER_NONE)
		return;

	voe_editor_notice_clear(&session->notice);
	session->armed = VOE_EDITOR_COMMAND_NONE;

	switch (result.action) {
	case VOE_EDITOR_BROWSER_NONE:
		return;

	case VOE_EDITOR_BROWSER_ENTERED:
		voe_editor_browser_enter(browser, result.name, &session->notice);
		return;

	case VOE_EDITOR_BROWSER_UP:
		voe_editor_browser_up(browser, &session->notice);
		return;

	case VOE_EDITOR_BROWSER_CANCEL:
		voe_editor_browser_hide(browser);
		return;

	case VOE_EDITOR_BROWSER_CONFIRM:
		if (browser->mode == VOE_EDITOR_BROWSER_OPEN) {
			voe_editor_project *opened = voe_editor_project_new_opened(
				browser->folder, &session->notice);

			if (opened == NULL)
				return;

			voe_editor_project_destroy(session->project);
			session->project = opened;
			session->replaced = true;
			scene->world = opened->world;
			scene->selected = (voe_ecs_entity){ 0 };

			if (!voe_editor_last_project_write(opened->folder))
				voe_editor_notice_set(
					&session->notice,
					"could not remember %s as the last project opened",
					opened->folder);

			voe_editor_browser_hide(browser);
			return;
		}

		// SAVE MODE. The project is still session->project's own —
		// unlike OPEN, nothing here replaces it — voe_editor_project_save
		// writes it into browser->folder and, on success, adopts that
		// folder as project->folder itself (project.h). A failure,
		// "is not empty" included, leaves why set by the save and the
		// browser open on the folder that refused it.
		if (!voe_editor_project_save(session->project, browser->folder,
					     &session->notice))
			return;

		if (!voe_editor_last_project_write(session->project->folder))
			voe_editor_notice_set(
				&session->notice,
				"could not remember %s as the last project opened",
				session->project->folder);

		voe_editor_browser_hide(browser);
		return;

	case VOE_EDITOR_BROWSER_MAKE_FOLDER:
		voe_editor_browser_make_folder(browser, result.name,
					       &session->notice);
		return;
	}
}
