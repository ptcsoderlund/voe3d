// Play: the project's game tree written, configured and built as it needs,
// then the game started, and a second press ending whichever step runs. The
// session carries one voe_editor_play and sends it PLAY (session.h); the loop
// polls it once a frame.
//
// A PROGRAM OF ITS OWN (ADR-0187). Every step is a child process from
// platform/process.h, polled without blocking, so the editor keeps drawing
// and answering while CMake, Ninja or the game runs.
//
// THE STAGES (ADR-0235). IDLE, then CONFIGURING when <project>/Build/debug has
// no CMake cache yet, then BUILDING, then RUNNING. The first Play in a project
// configures the tree and compiles the engine from its source, which takes a
// while, so the label reads "Building" for as long as either step runs; every
// later Play finds the cache and rebuilds only what changed.
//
// A FAILED BUILD IS ONE LINE ON STDERR (ADR-0234). A configure or build that
// ends non-zero, or a game that will not start, prints one `voe_editor: …`
// line naming the step and returns to idle; no notice, until the editor shows
// the compiler's errors. Only what refuses Play at the press (an untitled
// project, a refused write or start) is a notice.
//
// NOTHING HERE SAVES OR MARKS THE PROJECT (ADR-0188). The tree's scene.c is
// the world as it was at the press, unsaved edits included; later edits never
// reach the running game — a person stops and presses Play again.
//
// Constraints: one play runs one step at a time. Its arena exists only while
// not idle and holds the project's folder copied at the press, so the play
// does not depend on the project outliving it. A caller ends a play that is
// not idle before its own exit (voe_editor_play_end).
#pragma once

#include "notice.h"
#include "project.h"

#include <base/arena.h>

#include <platform/process.h>

typedef enum {
	VOE_EDITOR_PLAY_IDLE,
	VOE_EDITOR_PLAY_CONFIGURING,
	VOE_EDITOR_PLAY_BUILDING,
	VOE_EDITOR_PLAY_RUNNING,
} voe_editor_play_stage;

// Zeroed is idle: no process, no arena, no folder.
typedef struct {
	voe_editor_play_stage stage;
	voe_platform_process process;
	// The play's own memory, NULL while idle.
	voe_base_arena *arena;
	// The project's absolute folder, copied into arena at the press.
	const char *folder;
} voe_editor_play;

// Asserts idle. An untitled project gets a notice asking to save it once and
// nothing else (there is no folder to build in, ADR-0237). Otherwise writes
// the game tree and starts the configure or, when it is done already, the
// build. A refused write or start: why says so and play is idle again.
void voe_editor_play_start(voe_editor_play *play,
			   const voe_editor_project *project,
			   voe_editor_notice *why);

// Never blocks. Moves a step that ended 0 on to the next; a step that failed
// prints one stderr line and goes idle, as does a game that ended. Idle is a
// no-op.
void voe_editor_play_poll(voe_editor_play *play);

// Ends whatever runs, with everything it started, and goes idle. Idle is a
// no-op.
void voe_editor_play_end(voe_editor_play *play);

// "Play" while idle, "Building" while configuring or building, "Stop" while
// the game runs.
const char *voe_editor_play_label(const voe_editor_play *play);
