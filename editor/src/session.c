// The refuse-once rule, the seven commands, a refresh started, polled and its
// library swapped in, Ship started after it and polled, a failed build's
// Errors panel shown, and what a browser
// action does to the session. See the header for what each one does and why
// only CLOSE ever answers true.
#include "session.h"

#include "code.h"
#include "game_tree.h"
#include "last_project.h"

#include <base/arena.h>
#include <base/assert.h>
#include <base/report.h>

#include <ecs/world.h>

#include <scene/identity_component.h>

// A step's working memory: the library's path and, for the compare, both
// libraries' bytes. A block size, not a limit (base/arena.h).
#define SESSION_SCRATCH (64u * 1024u)

// Whether the project has a folder whose Code/ holds a .c file.
static bool session_has_code(const voe_editor_project *project)
{
	voe_base_arena *scratch;
	bool has;

	VOE_BASE_ASSERT(project != NULL, "asking no project for code");
	if (project->folder == NULL)
		return false;
	scratch = voe_base_arena_new(SESSION_SCRATCH);
	VOE_BASE_ASSERT(scratch != NULL, "no scratch to look for code in");
	has = voe_editor_game_tree_has_code(project->folder, scratch);
	voe_base_arena_destroy(scratch);
	return has;
}

// The Errors panel shown from the project's Build/build.log. A project with
// no folder has no log, and a failed build always had one.
static void session_errors_show(voe_editor_session *session)
{
	voe_base_arena *scratch;

	VOE_BASE_ASSERT(session->project != NULL, "showing errors of no project");
	if (session->project->folder == NULL)
		return;
	scratch = voe_base_arena_new(SESSION_SCRATCH);
	VOE_BASE_ASSERT(scratch != NULL, "no scratch to name the log in");
	voe_editor_errors_show(&session->errors,
			       voe_editor_game_tree_log(session->project->folder,
							scratch));
	voe_base_arena_destroy(scratch);
}

// Play started now, the last failure's panel hidden; a refusal is the notice.
static void session_play_start(voe_editor_session *session)
{
	VOE_BASE_ASSERT(session->play.stage == VOE_EDITOR_PLAY_IDLE,
			"starting a play over a running one");
	voe_editor_errors_hide(&session->errors);
	voe_editor_play_start(&session->play, session->project,
			      &session->notice);
	VOE_BASE_ASSERT(!session->errors.showing, "a started play shows errors");
}

// A refresh started now, so a due one is forgotten and the last failure's
// panel hidden; a refusal is the notice.
static void session_refresh_start(voe_editor_session *session)
{
	VOE_BASE_ASSERT(session->refresh.stage == VOE_EDITOR_REFRESH_IDLE,
			"starting a refresh over a running one");
	session->refresh_due = false;
	voe_editor_errors_hide(&session->errors);
	voe_editor_refresh_start(&session->refresh, session->project,
				 &session->notice);
	VOE_BASE_ASSERT(!session->refresh_due, "a started refresh still due");
}

// A ship started now, the last failure's panel hidden; a refusal is the notice.
static void session_ship_start(voe_editor_session *session)
{
	VOE_BASE_ASSERT(session->ship.stage == VOE_EDITOR_SHIP_IDLE,
			"starting a ship over a running one");
	voe_editor_errors_hide(&session->errors);
	voe_editor_ship_start(&session->ship, session->project,
			      &session->notice);
	VOE_BASE_ASSERT(!session->errors.showing, "a started ship shows errors");
}

// A ship runs or is to follow the running refresh, so nothing else may build.
static bool session_ship_busy(const voe_editor_session *session)
{
	return session->ship.stage != VOE_EDITOR_SHIP_IDLE ||
	       session->ship_after;
}

// The refresh and what was to follow it ended, before the project changes.
static void session_refresh_end(voe_editor_session *session)
{
	voe_editor_refresh_end(&session->refresh);
	session->play_after = false;
	session->ship_after = false;
	VOE_BASE_ASSERT(session->refresh.stage == VOE_EDITOR_REFRESH_IDLE,
			"an ended refresh still runs");
}

// The entity carrying id among world's authored rows, or a zeroed one when
// none does, which clears the selection.
static voe_ecs_entity session_entity_of(const voe_ecs_world *world, uint64_t id)
{
	const voe_scene_identity *rows = voe_scene_identity_rows(world);
	const voe_ecs_entity *entities = voe_scene_identity_entities(world);
	uint32_t count = voe_scene_identity_count(world);
	uint32_t i;

	VOE_BASE_ASSERT(world != NULL, "looking for an id in no world");
	for (i = 0; i < count; i++)
		if (rows[i].id == id)
			return entities[i];
	return (voe_ecs_entity){ 0 };
}

// The built library loaded and the world swapped for one made with it: 1 when
// swapped, 0 when its bytes equal the loaded one's, -1 with the notice saying
// why and the old world kept. The loaded copy's path lives in the project's
// arena, beside the code the project then owns.
static int session_code_swap(voe_editor_session *session,
			     voe_editor_scene *scene)
{
	voe_editor_project *project = session->project;
	voe_base_arena *scratch = voe_base_arena_new(SESSION_SCRATCH);
	const voe_scene_identity *identity;
	voe_ecs_entity selected = { 0 };
	voe_editor_code code;
	const char *built;
	bool was_selected;
	uint64_t id = 0;
	int result = -1;

	VOE_BASE_ASSERT(project != NULL && project->folder != NULL,
			"swapping code into a project with no folder");
	VOE_BASE_ASSERT(scratch != NULL, "no scratch to swap code with");

	built = voe_editor_game_tree_library(project->folder, scratch);
	if (voe_editor_code_same(built, &project->code, scratch)) {
		result = 0;
		goto destroy;
	}
	if (!voe_editor_code_open(built, project->folder, ++session->loads,
				  project->arena, &code, &session->notice))
		goto destroy;

	// Noted as an authored id: no handle survives the new world.
	identity = voe_scene_identity_get(project->world,
					  voe_editor_scene_selected(scene));
	was_selected = identity != NULL;
	if (was_selected)
		id = identity->id;
	voe_base_report_error_clear();
	if (!voe_editor_project_code_set(project, code, &session->notice))
		goto destroy;
	scene->world = project->world;
	if (was_selected)
		selected = session_entity_of(project->world, id);
	voe_editor_scene_select(scene, selected);
	voe_editor_scene_picker_close(scene);
	voe_editor_scene_dropdown_close(scene);
	result = 1;
destroy:
	voe_base_arena_destroy(scratch);
	return result;
}

bool voe_editor_session_step(voe_editor_session *session,
			     voe_editor_scene *scene)
{
	bool play;
	bool ship;
	int swapped;

	VOE_BASE_ASSERT(session != NULL && session->project != NULL,
			"stepping a session with no project");
	VOE_BASE_ASSERT(scene != NULL, "stepping a session with no scene");

	if (session->refresh.stage == VOE_EDITOR_REFRESH_IDLE) {
		// A due refresh waits out a ship: one build at a time.
		if (!session->refresh_due ||
		    session->ship.stage != VOE_EDITOR_SHIP_IDLE)
			return false;
		if (session_has_code(session->project))
			session_refresh_start(session);
		session->refresh_due = false;
		return false;
	}

	switch (voe_editor_refresh_poll(&session->refresh)) {
	case VOE_EDITOR_REFRESH_RUNNING:
		return false;
	case VOE_EDITOR_REFRESH_FAILED:
		session->play_after = false;
		session->ship_after = false;
		voe_editor_notice_set(
			&session->notice,
			"The project's code did not build — see Build/build.log");
		session_errors_show(session);
		return false;
	case VOE_EDITOR_REFRESH_BUILT:
		break;
	}

	play = session->play_after;
	ship = session->ship_after;
	session->play_after = false;
	session->ship_after = false;
	swapped = session_code_swap(session, scene);
	if (swapped >= 0 && play && session->play.stage == VOE_EDITOR_PLAY_IDLE)
		session_play_start(session);
	if (swapped >= 0 && ship && session->ship.stage == VOE_EDITOR_SHIP_IDLE)
		session_ship_start(session);
	return swapped > 0;
}

void voe_editor_session_play_poll(voe_editor_session *session)
{
	bool failed;

	VOE_BASE_ASSERT(session != NULL && session->project != NULL,
			"polling the play of a session with no project");

	failed = voe_editor_play_poll(&session->play);
	if (failed)
		session_errors_show(session);
	VOE_BASE_ASSERT(!failed || session->play.stage == VOE_EDITOR_PLAY_IDLE,
			"a failed play still runs");
}

const char *voe_editor_session_play_label(const voe_editor_session *session)
{
	VOE_BASE_ASSERT(session != NULL, "labelling no session's Play");

	if (session->play_after &&
	    session->refresh.stage != VOE_EDITOR_REFRESH_IDLE)
		return "Building";
	return voe_editor_play_label(&session->play);
}

void voe_editor_session_ship_poll(voe_editor_session *session)
{
	voe_editor_notice told = { 0 };

	VOE_BASE_ASSERT(session != NULL && session->project != NULL,
			"polling the ship of a session with no project");

	if (session->ship.stage == VOE_EDITOR_SHIP_IDLE)
		return;
	switch (voe_editor_ship_poll(&session->ship, &told)) {
	case VOE_EDITOR_SHIP_RUNNING:
		break;
	case VOE_EDITOR_SHIP_SHIPPED:
		session->notice = told;
		break;
	case VOE_EDITOR_SHIP_FAILED:
		session_errors_show(session);
		break;
	}
}

const char *voe_editor_session_ship_label(const voe_editor_session *session)
{
	VOE_BASE_ASSERT(session != NULL, "labelling no session's Ship");

	if (session->ship_after &&
	    session->refresh.stage != VOE_EDITOR_REFRESH_IDLE)
		return "Shipping";
	return voe_editor_ship_label(&session->ship);
}

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
		session_refresh_end(session);
		voe_editor_ship_end(&session->ship);
		return true;

	case VOE_EDITOR_COMMAND_PLAY:
		// Never arms: Play takes the world as it is, unsaved or not,
		// after the code it runs is in, and a second press is Stop.
		if (session->play.stage != VOE_EDITOR_PLAY_IDLE) {
			voe_editor_play_end(&session->play);
		} else if (session_ship_busy(session)) {
			voe_editor_notice_set(&session->notice,
					      "Ship is building — Play once it is done");
		} else if (session->refresh.stage != VOE_EDITOR_REFRESH_IDLE) {
			if (session->play_after)
				voe_editor_refresh_end(&session->refresh);
			session->play_after = !session->play_after;
		} else if (session_has_code(session->project)) {
			session_refresh_start(session);
			session->play_after = session->refresh.stage !=
					      VOE_EDITOR_REFRESH_IDLE;
		} else {
			session_play_start(session);
		}
		return false;

	case VOE_EDITOR_COMMAND_REFRESH:
		if (session_ship_busy(session))
			voe_editor_notice_set(&session->notice,
					      "Ship is building — Refresh once it is done");
		else if (session->refresh.stage == VOE_EDITOR_REFRESH_IDLE)
			session_refresh_start(session);
		return false;

	case VOE_EDITOR_COMMAND_SHIP:
		// Never arms, and one build at a time in Build/game/: a press
		// while shipping does nothing, a Play build is waited out and
		// any other refresh is joined (ADR-0264 point 5).
		if (session_ship_busy(session))
			return false;
		if (session->play.stage == VOE_EDITOR_PLAY_CONFIGURING ||
		    session->play.stage == VOE_EDITOR_PLAY_BUILDING ||
		    (session->play_after &&
		     session->refresh.stage != VOE_EDITOR_REFRESH_IDLE)) {
			voe_editor_notice_set(&session->notice,
					      "Play is building — Ship once it is done");
		} else if (session->refresh.stage != VOE_EDITOR_REFRESH_IDLE) {
			session->ship_after = true;
		} else if (session_has_code(session->project)) {
			session_refresh_start(session);
			session->ship_after = session->refresh.stage !=
					      VOE_EDITOR_REFRESH_IDLE;
		} else {
			session_ship_start(session);
		}
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

		session_refresh_end(session);
		voe_editor_ship_end(&session->ship);
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

			session_refresh_end(session);
			voe_editor_ship_end(&session->ship);
			session->refresh_due = true;
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

		session->refresh_due = true;
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
