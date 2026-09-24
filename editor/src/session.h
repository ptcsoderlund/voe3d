// The project being worked on, its notice, its Play, and the refuse-once rule
// that keeps unsaved work from being thrown away by one click or one press of
// a shortcut. The commands are New, Open, Save, Close and Play.
//
// ONE ARMED COMMAND, AND IT IS THE WHOLE OF THE RULE. `voe_editor_session_do`
// is given a command the top bar or a shortcut just asked for. With the
// project unsaved, CLOSE, NEW and OPEN are refused the first time — the notice
// says so and `armed` remembers which command it was, so the very next time
// the SAME command arrives it goes ahead instead. EVERY CALL STARTS BY
// CLEARING THE NOTICE AND DISARMING, whether this is the command that was
// armed or a different one entirely: only a refusal below puts a notice back,
// for the one command it just refused, so a command that goes ahead — the
// first time there was nothing to refuse, or the second time the refused one
// is repeated — always leaves the bar with nothing left to explain. `armed`
// is consumed exactly once, on every call — either it matched and the command
// went ahead, or it did not and is forgotten — so it never survives past the
// one command it was for.
//
// NEW REPLACES THE SESSION'S PROJECT ITSELF, ONCE ALLOWED. It destroys the old
// one and puts a fresh untitled project in its place — which is project.h's
// "build a whole new project, then discard the old one" — and clears `scene`'s
// selection, because an entity from the discarded world is not a selection in
// the new one.
//
// OPEN, ONCE ALLOWED, SHOWS THE BROWSER IN OPEN MODE, AND DOES NOTHING ELSE TO
// THE PROJECT ITSELF. What a chosen folder does — voe_editor_project_new_opened
// on it, replacing session->project on success — is
// voe_editor_session_browser_do's, carried out on the browser's own Confirm,
// which is not a command this function is ever asked for.
//
// SAVE NEVER ARMS. An opened project (session->project->folder is not NULL)
// is written back over itself, through voe_editor_project_save's second
// argument being NULL, and a failure lands in the notice the same way an open
// or a close's refusal does. AN UNTITLED PROJECT'S SAVE SHOWS THE BROWSER IN
// SAVE MODE INSTEAD, there being nowhere yet to write to; what its own
// Confirm ("Save here") does to the project is
// voe_editor_session_browser_do's, the same shape OPEN's Confirm already
// has.
//
// PLAY NEVER ARMS AND IS NEVER REFUSED FOR UNSAVED WORK: it plays the world
// as it is (ADR-0188). Idle, it starts session->play (play.h); a second press
// while building or running is Stop and ends it. A CLOSE THAT GOES AHEAD ENDS
// THE PLAY FIRST, so the editor never leaves a build or a game behind.
//
// voe_editor_session_edited IS THE OTHER HALF OF WHAT DISARMS. An edit in the
// inspector is not a command this file was asked to do, so nothing above
// would otherwise notice it; the caller that reads a non-zero count of
// replaced components (inspector.h) calls this instead, which marks the
// project unsaved, clears whatever notice was showing and disarms — the same
// "anything else done" the header above describes, for the one kind of change
// that never goes through voe_editor_session_do at all. A view's camera drag
// calls nothing here, because moving a camera does not change the project.
//
// `replaced` IS A FLAG AND NOT A RETURN VALUE. A different project lands in
// session->project in two places — NEW in voe_editor_session_do and the
// browser's Confirm in OPEN mode in voe_editor_session_browser_do — which are
// two calls, one of them made from interface.c, where a return value would
// have to be carried back through a drawing call that has nothing to do with
// it. The loop is the only thing that needs to hear about it, so it reads the
// flag and clears it (main.c empties the undo history on it).
#pragma once

#include "browser.h"
#include "notice.h"
#include "play.h"
#include "project.h"
#include "scene.h"

// The five things a person can ask the session to do, and NONE for "nothing
// was asked this frame" — which is never a valid argument to
// voe_editor_session_do, only the value `armed` rests at between commands.
typedef enum {
	VOE_EDITOR_COMMAND_NONE,
	VOE_EDITOR_COMMAND_NEW,
	VOE_EDITOR_COMMAND_OPEN,
	VOE_EDITOR_COMMAND_SAVE,
	VOE_EDITOR_COMMAND_CLOSE,
	VOE_EDITOR_COMMAND_PLAY,
} voe_editor_command;

// Zeroed is a session with no project yet and nothing armed — the caller sets
// `project` before the first voe_editor_session_do, exactly as scene.world is
// set from outside (scene.h).
typedef struct {
	voe_editor_project *project;
	voe_editor_notice notice;
	voe_editor_command armed;
	// Idle while zeroed; ended by a CLOSE that goes ahead.
	voe_editor_play play;
	// A different project is in session->project, set by whichever call
	// put it there and cleared by whoever acts on it.
	bool replaced;
} voe_editor_session;

// Carries out command, or refuses it once — see the header above. `scene` is
// where NEW's fresh world and cleared selection land; `browser` is what OPEN
// shows; neither is touched by any other command.
//
// TRUE ONLY FOR A CLOSE THAT GOES AHEAD. Every other command, refused or not,
// answers false: NEW replaces `session->project`, OPEN shows `browser`, SAVE
// writes to the project and PLAY starts or ends session->play, none of which
// the caller has to be told happened, and a refused command is exactly the case in which nothing may go
// ahead. A caller that asked for CLOSE and got false carries on running; one
// that got true closes.
bool voe_editor_session_do(voe_editor_session *session, voe_editor_scene *scene,
			   voe_editor_browser *browser,
			   voe_editor_command command);

// An edit reached the project outside any command above — the inspector's.
// Marks session->project unsaved, clears the notice and disarms, exactly as
// "anything else done" does in voe_editor_session_do.
void voe_editor_session_edited(voe_editor_session *session);

// Carries out what browser reported this frame — see browser.h's
// voe_editor_browser_clicks_read. Entering a row, going up and making a
// folder (MAKE_FOLDER, the name field's own Enter or its Make folder button)
// are browser.c's own to do, given session->notice to write a failure into;
// Cancel hides the browser and nothing else. CONFIRM IS THIS FILE'S OWN IN
// BOTH MODES, the same shape session_do's other commands have. In OPEN mode:
// voe_editor_project_new_opened on browser->folder, and on success
// session->project, scene->world and scene->selected are replaced exactly as
// NEW replaces them, the last project is remembered (a failure to write that
// is a notice and not a refusal), and the browser hides; on failure the
// notice is why, the browser stays and session->project is untouched. IN
// SAVE MODE: voe_editor_project_save(session->project, browser->folder, why)
// — the first save a project this session built untitled ever gets — and on
// success the same "remembered, then hidden" as OPEN's; on failure,
// INCLUDING "IS NOT EMPTY", the notice is why, nothing was written and the
// browser stays, exactly as OPEN's own failure leaves session->project
// untouched.
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
