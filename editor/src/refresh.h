// Refresh: the project's code built as a library, the LIBRARY kind of
// game_tree.h, so the session can load it (code.h) and swap the world for one
// made with it (project.h). The session carries one voe_editor_refresh, starts
// it and polls it once a frame (session.h); what a built library does is the
// session's, not this file's (ADR-0242 point 7).
//
// PLAY'S SHAPE, ONE KIND DOWN (play.h). Every step is a child process from
// platform/process.h, polled without blocking. IDLE, then CONFIGURING when
// <project>/Build/editor has no CMake cache yet, then BUILDING; a build that
// ends 0 is BUILT and the refresh goes idle again.
//
// A FAILED STEP IS ONE LINE ON STDERR AND A LOG (ADR-0240, 0242 point 8). A
// start empties <project>/Build/build.log and every step writes its output
// there. A step that ends non-zero prints one `voe_editor: …` line naming the
// step and the log and the poll answers FAILED. Only what refuses at the start
// (an untitled project, a refused write or start) is a notice.
//
// Constraints: one step at a time. The arena exists only while not idle and
// holds the project's folder copied at the start. A caller ends a refresh that
// is not idle before its own exit (voe_editor_refresh_end).
#pragma once

#include "notice.h"
#include "project.h"

#include <base/arena.h>

#include <platform/process.h>

typedef enum {
	VOE_EDITOR_REFRESH_IDLE,
	VOE_EDITOR_REFRESH_CONFIGURING,
	VOE_EDITOR_REFRESH_BUILDING,
} voe_editor_refresh_stage;

// What one poll of a refresh that is not idle found.
typedef enum {
	VOE_EDITOR_REFRESH_RUNNING,
	VOE_EDITOR_REFRESH_BUILT,
	VOE_EDITOR_REFRESH_FAILED,
} voe_editor_refresh_result;

// Zeroed is idle: no process, no arena, no folder.
typedef struct {
	voe_editor_refresh_stage stage;
	voe_platform_process process;
	// The refresh's own memory, NULL while idle.
	voe_base_arena *arena;
	// The project's absolute folder, copied into arena at the start.
	const char *folder;
} voe_editor_refresh;

// Asserts idle. An untitled project gets a notice asking to save it once and
// nothing else. Otherwise writes the game tree, empties the log and starts the
// LIBRARY configure or, when it is done already, the build. A refused write or
// start: why says so and refresh is idle again.
void voe_editor_refresh_start(voe_editor_refresh *refresh,
			      const voe_editor_project *project,
			      voe_editor_notice *why);

// Never blocks; asserts not idle. RUNNING while a step runs or a configure
// moved on to the build; BUILT when the build ended 0, the library then at
// voe_editor_game_tree_library; FAILED after the one stderr line. Idle again
// after BUILT or FAILED.
voe_editor_refresh_result voe_editor_refresh_poll(voe_editor_refresh *refresh);

// Ends whatever runs and goes idle. Idle is a no-op.
void voe_editor_refresh_end(voe_editor_refresh *refresh);

// "Refresh" while idle, "Refreshing" while configuring or building.
const char *voe_editor_refresh_label(const voe_editor_refresh *refresh);
