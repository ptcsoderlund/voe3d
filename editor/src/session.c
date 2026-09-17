// The refuse-once rule and the four commands. See the header for what each one
// does and why only CLOSE ever answers true.
#include "session.h"

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
		return true;

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
		// Allowed, once past the refusal above — the browser that
		// shows and what a chosen folder does to the project are
		// tasks 13 and 14's; there is nothing more to do here yet.
		return false;

	case VOE_EDITOR_COMMAND_SAVE:
		// An untitled project has nowhere to write to yet — the
		// browser that gives it one is task 13's — so Save does
		// nothing until then.
		if (session->project->folder != NULL)
			(void)voe_editor_project_save(session->project, NULL,
						      &session->notice);
		return false;

	case VOE_EDITOR_COMMAND_NONE:
		break;
	}

	return false;
}
