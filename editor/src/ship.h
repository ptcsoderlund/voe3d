// Ship: the project's game built in release, the RELEASE kind of game_tree.h,
// and installed into the shipped folder, whose path is
// voe_editor_game_tree_shipped (ADR-0264). The session carries one
// voe_editor_ship, starts it and polls it once a frame.
//
// REFRESH'S SHAPE WITH TWO MORE STEPS (refresh.h). Every step is a child
// process from platform/process.h, polled without blocking. IDLE, then
// CONFIGURING when <project>/Build/release has no CMake cache yet, then
// BUILDING, CLEARING (the old shipped folder removed) and INSTALLING; an
// install that ends 0 is SHIPPED and the ship goes idle again.
//
// A FAILED BUILD LEAVES THE OLD SHIPPED FOLDER AS IT WAS: CLEARING comes only
// after a build that ended 0, so the last good ship survives a broken one.
//
// NOTHING SAVES THE PROJECT (ADR-0188): the tree is written with the scene as
// it is at the start, unsaved edits included, and project->unsaved stays.
//
// A FAILED STEP IS ONE LINE ON STDERR AND A LOG (ADR-0240). A start empties
// <project>/Build/build.log and every step writes its output there. A step
// that ends non-zero prints one `voe_editor: …` line naming the step and the
// log and the poll answers FAILED. Only what refuses at the start (an untitled
// project, a refused write or start) is a notice.
//
// Constraints: one step at a time. The arena exists only while not idle and
// holds the project's folder copied at the start. A caller ends a ship that is
// not idle before its own exit (voe_editor_ship_end).
#pragma once

#include "notice.h"
#include "project.h"

#include <base/arena.h>

#include <platform/process.h>

typedef enum {
	VOE_EDITOR_SHIP_IDLE,
	VOE_EDITOR_SHIP_CONFIGURING,
	VOE_EDITOR_SHIP_BUILDING,
	VOE_EDITOR_SHIP_CLEARING,
	VOE_EDITOR_SHIP_INSTALLING,
} voe_editor_ship_stage;

// What one poll of a ship that is not idle found.
typedef enum {
	VOE_EDITOR_SHIP_RUNNING,
	VOE_EDITOR_SHIP_SHIPPED,
	VOE_EDITOR_SHIP_FAILED,
} voe_editor_ship_result;

// Zeroed is idle: no process, no arena, no folder.
typedef struct {
	voe_editor_ship_stage stage;
	voe_platform_process process;
	// The ship's own memory, NULL while idle.
	voe_base_arena *arena;
	// The project's absolute folder, copied into arena at the start.
	const char *folder;
} voe_editor_ship;

// Asserts idle. An untitled project gets a notice asking to save it once and
// nothing else. Otherwise writes the game tree, empties the log and starts the
// RELEASE configure or, when it is done already, the build. A refused write or
// start: why says so and ship is idle again.
void voe_editor_ship_start(voe_editor_ship *ship,
			   const voe_editor_project *project,
			   voe_editor_notice *why);

// Never blocks; asserts not idle. RUNNING while a step runs or an ended step
// moved on to the next; SHIPPED when the install ended 0, told then saying
// "Shipped to <the shipped folder>"; FAILED after the one stderr line. Idle
// again after SHIPPED or FAILED.
voe_editor_ship_result voe_editor_ship_poll(voe_editor_ship *ship,
					    voe_editor_notice *told);

// Ends whatever runs and goes idle. Idle is a no-op.
void voe_editor_ship_end(voe_editor_ship *ship);

// "Ship" while idle, "Shipping" otherwise.
const char *voe_editor_ship_label(const voe_editor_ship *ship);
