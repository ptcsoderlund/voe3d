// The project being worked on, its notice, Play, Refresh and Ship, open prefab
// and the refuse-once rule: no one click or shortcut throws unsaved work away.
//
// ONE ARMED COMMAND IS THE WHOLE OF THE RULE. With unsaved work, CLOSE, NEW,
// OPEN and BACK are refused the first time: the notice says so and `armed`
// remembers which, so the very next time the SAME command arrives it goes
// ahead. EVERY CALL TO voe_editor_session_do STARTS BY CLEARING THE NOTICE AND
// DISARMING, so `armed` never outlives the one command it was for. CLOSE, NEW
// and OPEN count the open prefab and the level set aside for it (0283 point 8).
//
// NEW, ONCE ALLOWED, and OPEN's Confirm (voe_editor_session_browser_do) only
// set `load_due`; voe_editor_session_load, a frame later behind a splash frame
// (0356), puts the fresh untitled or the opened project in place, clears
// `scene`'s selection and ends a running refresh and ship. OPEN shows the
// browser beside the project's folder, or for an untitled scene the last
// project, its row chosen (0343). SAVE NEVER ARMS: an opened project (or its
// open prefab) is written over itself; an untitled one shows the browser in
// SAVE, starting as OPEN's does, and Save here still means the shown folder.
//
// A PREFAB IS OPENED BY voe_editor_session_prefab_open, refused while one is
// open, and left by BACK, refused once while it is unsaved; with none open BACK
// does nothing. Both clear the selection; `prefab_opened` and `prefab_closed`
// tell the loop, which swaps undo.h's lines on them.
//
// PLAY NEVER ARMS AND IS NEVER REFUSED FOR UNSAVED WORK (ADR-0188), only while
// a prefab is open (Back first). Idle with code in `Code/`, a refresh starts
// with `play_after` and Play once the code is in (ADR-0242 point 7); with no
// code session->play (play.h) starts at once. A press during that refresh ends
// it, during any other sets `play_after`, and while building or running is
// Stop. A CLOSE THAT GOES AHEAD ENDS THE PLAY, THE REFRESH AND THE SHIP FIRST.
//
// SHIP NEVER ARMS (ADR-0264): Play's shape, `ship_after` its `play_after`,
// refused while a prefab is open, and a press while shipping does nothing. ONE
// BUILD AT A TIME: Ship waits with a notice while Play builds or a refresh runs
// for Play, and joins any other refresh; Play's build and Refresh wait with a
// notice while a ship runs or follows a refresh. A ship never stops a game.
//
// REFRESH NEVER ARMS: it starts session->refresh now (refresh.h), unless one
// runs. `refresh_due` asks for one the next step, set wherever a project with
// a folder lands: Open's load, a first Save, and startup.h.
//
// voe_editor_session_step IS WHERE A REFRESH LANDS, once a frame at its top,
// before the undo take and the structural queue: a due one started if the
// project has code, a running one polled. BUILT loads the library unless its
// bytes equal the loaded one's (code.h), swaps the world (project.h), re-finds
// the selection by authored id and closes the picker and dropdown. A failure
// says so and keeps the old world; `play_after` and `ship_after` are dropped.
//
// A FAILED BUILD SHOWS `errors` (errors.h) from Build/build.log: a refresh
// that answers FAILED, or a Play or Ship whose step failed (the two polls). A
// refresh, Play or Ship that starts hides it.
//
// voe_editor_session_edited IS THE OTHER HALF OF WHAT DISARMS: the
// inspector's edits are no command, so its caller calls this, which marks the
// project unsaved, clears the notice and disarms.
//
// `replaced` AND `saved` ARE FLAGS, NOT RETURN VALUES: another project loaded
// (voe_editor_session_load), or this one written by SAVE or Save's Confirm;
// main.c clears each, emptying undo on one and saving landscapes on the other.
#pragma once

#include "assets_ask.h"
#include "browser.h"
#include "errors.h"
#include "notice.h"
#include "play.h"
#include "project.h"
#include "refresh.h"
#include "scene.h"
#include "ship.h"

#include <stdint.h>

// The longest folder Open's Confirm carries to the load, its terminator
// included; a longer one is refused with a notice.
#define VOE_EDITOR_SESSION_FOLDER 4096

// The eight things a person can ask the session to do, and NONE for "nothing
// was asked this frame" — which is never a valid argument to
// voe_editor_session_do, only the value `armed` rests at between commands.
typedef enum {
	VOE_EDITOR_COMMAND_NONE,
	VOE_EDITOR_COMMAND_NEW,
	VOE_EDITOR_COMMAND_OPEN,
	VOE_EDITOR_COMMAND_SAVE,
	VOE_EDITOR_COMMAND_CLOSE,
	VOE_EDITOR_COMMAND_PLAY,
	VOE_EDITOR_COMMAND_REFRESH,
	VOE_EDITOR_COMMAND_SHIP,
	VOE_EDITOR_COMMAND_BACK,
} voe_editor_command;

// Zeroed is a session with no project yet and nothing armed — the caller sets
// `project` before the first voe_editor_session_do, exactly as scene.world is
// set from outside (scene.h).
typedef struct voe_editor_session {
	voe_editor_project *project;
	voe_editor_notice notice;
	voe_editor_command armed;
	// Idle while zeroed; ended by a CLOSE that goes ahead.
	voe_editor_play play;
	// Idle while zeroed; ended by a CLOSE that goes ahead, NEW and OPEN.
	voe_editor_refresh refresh;
	// The running refresh starts Play once its code is in.
	bool play_after;
	// Idle while zeroed; ended by a CLOSE that goes ahead, NEW and OPEN.
	voe_editor_ship ship;
	// The running refresh starts Ship once its code is in.
	bool ship_after;
	// A refresh is to start at the next voe_editor_session_step.
	bool refresh_due;
	// How many libraries this run has loaded, the n of project-<n> (code.h).
	uint32_t loads;
	// Shown by a failed build, hidden by a refresh, Play or Ship that starts.
	voe_editor_errors errors;
	// The Assets panel's Delete question (assets_ask.h), opened and
	// answered by interface.c, closed first by Escape (frame_commands.c).
	voe_editor_assets_ask asking;
	// A different project is in session->project, set by whichever call
	// put it there and cleared by whoever acts on it.
	bool replaced;
	// The project was written by a SAVE or the browser's Save Confirm;
	// cleared by main.c once it has written the edited landscapes.
	bool saved;
	// A prefab was opened in the level's place, or Back left it; each set
	// by the call that did it and cleared by main.c.
	bool prefab_opened;
	bool prefab_closed;
	// NEW or OPEN's Confirm asked for a load, which main.c does next frame
	// through voe_editor_session_load. `load_folder` is Open's chosen
	// folder, copied out of the browser; empty is NEW's untitled project.
	bool load_due;
	char load_folder[VOE_EDITOR_SESSION_FOLDER];
} voe_editor_session;

// Carries out command, or refuses it once — see the header above. `scene` is
// where NEW's fresh world and cleared selection land; `browser` is what OPEN
// shows; neither is touched by any other command.
//
// TRUE ONLY FOR A CLOSE THAT GOES AHEAD. Every other command, refused or not,
// answers false: NEW sets `load_due`, OPEN shows `browser`, SAVE
// writes to the project, PLAY starts or ends session->play or a refresh,
// REFRESH starts one, SHIP starts a ship or a refresh and BACK goes back to the
// level (prefab_closed), none of which the caller has to be told happened, and a
// refused command is exactly the case in which nothing may go ahead. A caller
// that asked for CLOSE and got false carries on running; one that got true
// closes.
bool voe_editor_session_do(voe_editor_session *session, voe_editor_scene *scene,
			   voe_editor_browser *browser,
			   voe_editor_command command);

// Once a frame, before the undo take and the world step: a due refresh
// started if the project has code, a running one polled, and a built library
// loaded and swapped in, then Play or Ship if `play_after` or `ship_after` —
// see the header above. A due refresh waits while a ship runs.
// True when the world was swapped, so every entity handle outside `scene` is
// stale and scene->world is the new one.
bool voe_editor_session_step(voe_editor_session *session,
			     voe_editor_scene *scene);

// Once a frame: session->play polled (play.h), and a configure or build that
// failed on this poll shows `errors` from the project's build log.
void voe_editor_session_play_poll(voe_editor_session *session);

// `errors` shown from the project's Build/build.log, as a failed build shows
// it; panels.h's toggle calls it too. Nothing for a project with no folder.
void voe_editor_session_errors_show(voe_editor_session *session);

// "Building" while a refresh with `play_after` runs, else
// voe_editor_play_label.
const char *voe_editor_session_play_label(const voe_editor_session *session);

// Once a frame: a ship not idle polled (ship.h). SHIPPED sets the notice to
// what the poll told; FAILED shows `errors` from the project's build log.
void voe_editor_session_ship_poll(voe_editor_session *session);

// "Shipping" while a refresh with `ship_after` runs, else
// voe_editor_ship_label.
const char *voe_editor_session_ship_label(const voe_editor_session *session);

// An edit reached the project outside any command above — the inspector's.
// Marks session->project unsaved, clears the notice and disarms, exactly as
// "anything else done" does in voe_editor_session_do.
void voe_editor_session_edited(voe_editor_session *session);

// Opens the prefab at `path` under the project's folder in the level's place
// (voe_editor_project_prefab_open), clearing the notice and disarming as a
// command does. Refused with a notice while a prefab is open; a failure is the
// notice. On success `scene`'s selection is cleared, the picker and dropdown
// closed, any Play or Ship to follow a refresh dropped, and prefab_opened set.
void voe_editor_session_prefab_open(voe_editor_session *session,
				    voe_editor_scene *scene, const char *path);

// Carries out what browser reported this frame — see browser.h's
// voe_editor_browser_clicks_read. Entering a row, going up and making a
// folder (MAKE_FOLDER, the name field's own Enter or its Make folder button)
// are browser.c's own to do, given session->notice to write a failure into;
// Cancel hides the browser and nothing else. IMPORT_FILE copies the pressed
// file into the Assets panel's shown folder (voe_editor_assets_import, a
// failure in the notice) and hides the browser. CONFIRM IS THIS FILE'S OWN IN
// BOTH MODES, the same shape session_do's other commands have. In OPEN mode:
// browser->target copied into `load_folder`, `load_due` set and the browser
// hidden; voe_editor_session_load opens it next frame. A target too long to
// copy is a notice and the browser stays. IN SAVE MODE:
// voe_editor_project_save(session->project, browser->folder, why) — the first
// save a project this session built untitled ever gets — and on success the
// last project is remembered (a failure to write that is a notice and not a
// refusal), a refresh is due and the browser hides; on failure, INCLUDING "IS
// NOT EMPTY", the notice is why, nothing was written and the browser stays.
//
// EVERY BROWSER ACTION CLEARS THE NOTICE AND DISARMS, AS A COMMAND DOES — but
// only when one actually fired: a call with result.action ==
// VOE_EDITOR_BROWSER_NONE, which is every frame nothing was clicked, does
// nothing at all, so a listing failure's notice survives the frames in which
// nothing else happened to it.
void voe_editor_session_browser_do(voe_editor_session *session,
				   voe_editor_scene *scene,
				   voe_editor_browser *browser,
				   voe_editor_browser_result result);

// The load `load_due` asked for, cleared here: a fresh untitled project for
// NEW, voe_editor_project_new_opened on `load_folder` for Open. On success the
// running refresh and ship end, session->project and scene->world are
// replaced, the selection cleared and `replaced` set; an opened one is
// remembered as the last project (a failed write is a notice) and a refresh is
// due. A failed open's notice is why, and session->project is untouched.
void voe_editor_session_load(voe_editor_session *session,
			     voe_editor_scene *scene);
