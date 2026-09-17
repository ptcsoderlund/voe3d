// The project being worked on, its notice, and the refuse-once rule that keeps
// unsaved work from being thrown away by one click or one press of a shortcut.
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
// NEW AND OPEN, ONCE ALLOWED, TAKE THE SESSION'S PROJECT ITSELF. NEW destroys
// the old one and puts a fresh untitled project in its place — which is
// project.h's "build a whole new project, then discard the old one" — and
// clears `scene`'s selection, because an entity from the discarded world is
// not a selection in the new one. Open does nothing more than clear its own
// refusal today; the browser and what a chosen folder does to the project are
// tasks 13 and 14's.
//
// SAVE NEVER ARMS. An opened project (session->project->folder is not NULL)
// is written back over itself, through voe_editor_project_save's second
// argument being NULL, and a failure lands in the notice the same way an open
// or a close's refusal does. An untitled project's Save does nothing yet — the
// first save is the browser task's — so there is nothing here for it to be
// refused over.
//
// voe_editor_session_edited IS THE OTHER HALF OF WHAT DISARMS. An edit in the
// inspector is not a command this file was asked to do, so nothing above
// would otherwise notice it; the caller that reads a non-zero count of
// replaced components (inspector.h) calls this instead, which marks the
// project unsaved, clears whatever notice was showing and disarms — the same
// "anything else done" the header above describes, for the one kind of change
// that never goes through voe_editor_session_do at all. A view's camera drag
// calls nothing here, because moving a camera does not change the project.
#pragma once

#include "notice.h"
#include "project.h"
#include "scene.h"

// The four things a person can ask the session to do, and NONE for "nothing
// was asked this frame" — which is never a valid argument to
// voe_editor_session_do, only the value `armed` rests at between commands.
typedef enum {
	VOE_EDITOR_COMMAND_NONE,
	VOE_EDITOR_COMMAND_NEW,
	VOE_EDITOR_COMMAND_OPEN,
	VOE_EDITOR_COMMAND_SAVE,
	VOE_EDITOR_COMMAND_CLOSE,
} voe_editor_command;

// Zeroed is a session with no project yet and nothing armed — the caller sets
// `project` before the first voe_editor_session_do, exactly as scene.world is
// set from outside (scene.h).
typedef struct {
	voe_editor_project *project;
	voe_editor_notice notice;
	voe_editor_command armed;
} voe_editor_session;

// Carries out command, or refuses it once — see the header above. `scene` is
// where NEW's fresh world and cleared selection land; it is not touched by any
// other command.
//
// TRUE ONLY FOR A CLOSE THAT GOES AHEAD. Every other command, refused or not,
// answers false: NEW and OPEN act by replacing or leaving `session->project`
// and SAVE by writing to it, neither of which the caller has to be told
// happened, and a refused command is exactly the case in which nothing may go
// ahead. A caller that asked for CLOSE and got false carries on running; one
// that got true closes.
bool voe_editor_session_do(voe_editor_session *session, voe_editor_scene *scene,
			   voe_editor_command command);

// An edit reached the project outside any command above — the inspector's.
// Marks session->project unsaved, clears the notice and disarms, exactly as
// "anything else done" does in voe_editor_session_do.
void voe_editor_session_edited(voe_editor_session *session);
