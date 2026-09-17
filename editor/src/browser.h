// The editor's own file browser: a folder listing shown as a panel over the
// dock, used to choose a project to open (and, from task 14, a folder to save
// one into). There is no system dialog anywhere in this engine (ADR-0164's
// spec) — this is the whole of what stands in for one.
//
// IT OWNS ITS OWN ARENA, MADE ONCE AND CLEARED ON EVERY NAVIGATION. The
// folder's absolute path and its rows are pushed into `arena`; entering a row
// or going up clears the whole thing and rebuilds it from the new folder, so
// nothing here ever holds two folders' worth of names at once. `arena` is made
// the first time the browser is shown and lives for the rest of the program —
// there is no _destroy on a hide, because THE BROWSER KEEPS ITS FOLDER ACROSS
// SHOWINGS, FOR THE SESSION. Close it over one project's folder, choose
// Open again, and it opens back where it was left; only the very first
// showing has no folder yet and picks one (voe_platform_folder_home, or
// voe_platform_path_absolute(".") when the platform has no home to offer).
//
// A LISTING THAT FAILS CHANGES NOTHING. voe_editor_browser_enter and
// voe_editor_browser_up build the candidate folder, list it into a scratch
// arena of their own, and only touch `arena` — clearing it and copying the new
// rows in — once that listing has already succeeded. So a folder that stops
// being readable between one frame and the next (removed, permissions
// changed) is reported through `why` and leaves the browser exactly where it
// was, never with a folder it could not actually list.
//
// EVERY ROW IS A FOLDER, NEVER A FILE, AND NEVER A HIDDEN ONE — the same rule
// platform/folder.h's own listing states, applied here rather than
// re-decided. A row is marked when `<row>/project.voe3d` exists
// (voe_platform_file_exists), which is the only thing that tells a folder
// full of other folders from a project.
//
// THIS FILE CARRIES OUT NO COMMAND OF ITS OWN, exactly as topbar.h's buttons
// do not: voe_editor_browser_clicks_read says what fired — a row entered, Up,
// Confirm (with the folder it names), Cancel, or Escape read as a Cancel —
// and what a click MEANS, in particular what Confirm does to the project
// being worked on, is session.h's (voe_editor_session_browser_do). Entering a
// row and going up are the exception: they change nothing about a project, so
// this file carries them out itself, given the notice to write into on
// failure.
//
// THE THREE FIXED BUTTONS AND EVERY ROW ARE RECORDED AND READ BACK, exactly as
// the Scene panel's rows and the top bar's buttons are (scene.h, topbar.h): a
// `ui` widget answers what the pointer did to it only after voe_ui_frame_end,
// so voe_editor_browser_draw records where each one is and
// voe_editor_browser_clicks_read asks afterwards, inside the same window
// every other panel's own read uses.
//
// VOE_EDITOR_BROWSER_ROWS IS THE SAME ROOM THE SCENE PANEL GETS. A folder
// holding more subfolders than that scrolls instead of growing the list
// further — the scroll area already exists for exactly this — and thirty-two
// is already more than a person usefully points at without one.
#pragma once

#include "notice.h"

#include <base/arena.h>

#include <math/float2.h>

#include <ui/widgets.h>

#include <stdbool.h>
#include <stdint.h>

// How many folder rows one showing may hold — see the header above.
#define VOE_EDITOR_BROWSER_ROWS 32

// Which the browser is for. SAVE mode's name box and Make-folder button are
// task 14's; this file already carries the value because session.h has to
// say which one a showing is for.
typedef enum {
	VOE_EDITOR_BROWSER_OPEN,
	VOE_EDITOR_BROWSER_SAVE,
} voe_editor_browser_mode;

// One listed folder: its name (not its full path, into the browser's own
// arena) and the button drawn for it, and whether `<name>/project.voe3d`
// exists.
typedef struct {
	voe_ui_node node;
	const char *name;
	bool project;
} voe_editor_browser_row;

// The browser's whole state. Zeroed is a browser never shown: arena is NULL,
// folder is NULL, and showing is false — voe_editor_browser_show makes the
// arena the first time it is called.
typedef struct {
	bool showing;
	voe_editor_browser_mode mode;

	// Made on the first showing, cleared on every navigation and never
	// destroyed until the program ends — see the header above.
	voe_base_arena *arena;
	// The current folder, absolute, in `arena`. NULL until the first
	// showing has picked a starting folder.
	const char *folder;

	voe_editor_browser_row rows[VOE_EDITOR_BROWSER_ROWS];
	uint32_t row_count;

	// The three buttons besides the rows, recorded as voe_editor_topbar's
	// are.
	voe_ui_node up_button;
	voe_ui_node confirm_button;
	voe_ui_node cancel_button;
} voe_editor_browser;

// Shows browser in mode, keeping its folder if one is already set, or
// starting at the home folder — else the current one — the first time this is
// ever called. A listing that fails at this point is reported through why and
// the browser still shows, holding whatever folder and rows it had before
// (nothing, on a first showing).
void voe_editor_browser_show(voe_editor_browser *browser,
			     voe_editor_browser_mode mode,
			     voe_editor_notice *why);

// Hides browser. Its folder and rows are kept for the next showing — see the
// header above.
void voe_editor_browser_hide(voe_editor_browser *browser);

// Enters row name of browser's current folder. False and why set on a listing
// that fails, with browser's folder and rows untouched; true relists the
// entered folder. Asserts if browser has no folder yet.
void voe_editor_browser_enter(voe_editor_browser *browser, const char *name,
			      voe_editor_notice *why);

// Goes to the parent of browser's current folder. Does nothing at a root,
// where platform/path.h's own parent is NULL. Otherwise the same as
// voe_editor_browser_enter, one level up instead of down.
void voe_editor_browser_up(voe_editor_browser *browser, voe_editor_notice *why);

// Draws the browser as one anchored panel, filling top..size.y of `size`'s
// width — the area below the top bar, in the same column interface.c opens
// over the dock (interface.h) — which is what puts it over the dock and under
// nothing. The current path, an Up button, a scroll area of one button per
// row (marked rows read " — project"), then Confirm ("Open" in OPEN mode) and
// Cancel. Records every button into browser, read back by
// voe_editor_browser_clicks_read.
void voe_editor_browser_draw(voe_ui_context *ui, voe_editor_browser *browser,
			     float top, voe_math_float2 size);

typedef enum {
	VOE_EDITOR_BROWSER_NONE,
	VOE_EDITOR_BROWSER_ENTERED,
	VOE_EDITOR_BROWSER_UP,
	VOE_EDITOR_BROWSER_CONFIRM,
	VOE_EDITOR_BROWSER_CANCEL,
} voe_editor_browser_action;

// What fired this frame. `name` is the row entered, valid only when action is
// VOE_EDITOR_BROWSER_ENTERED, and a pointer into browser's own arena — read it
// before the next navigation clears that arena. Confirm names no folder of its
// own: browser->folder already is the one to act on.
typedef struct {
	voe_editor_browser_action action;
	const char *name;
} voe_editor_browser_result;

// Which button or row fired this frame, or VOE_EDITOR_BROWSER_NONE when none
// did. escape is this frame's Escape key edge, read as a Cancel the same as
// the Cancel button — the caller's, since a window is not this folder's to
// ask (ADR-0141 point 4). Called after voe_ui_frame_end and before the
// frame's arena is rewound, the one window in which a widget will answer.
voe_editor_browser_result
voe_editor_browser_clicks_read(const voe_ui_context *ui,
			       const voe_editor_browser *browser, bool escape);

// Frees the browser's own arena, when it ever made one. Called once, at
// shutdown.
void voe_editor_browser_destroy(voe_editor_browser *browser);
