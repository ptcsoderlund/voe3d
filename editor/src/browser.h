// The editor's own file browser: a folder listing shown as a panel over the
// dock, used to choose a project to open, a folder to save one into (SAVE,
// with a name row for a new folder), or a `.glb` to import into the Assets
// panel's folder (IMPORT, 0277 point 7). There is no system dialog anywhere
// in this engine (ADR-0164) — this is the whole of what stands in for one.
//
// IT OWNS ITS OWN ARENA, MADE ONCE AND CLEARED ON EVERY NAVIGATION. The
// folder's absolute path and its rows are pushed into `arena`; entering a row
// or going up clears it and rebuilds it from the new folder, so nothing here
// holds two folders' worth of names at once. `arena` lives from the first
// showing to the end of the program, because THE BROWSER KEEPS ITS FOLDER
// ACROSS SHOWINGS, FOR THE SESSION, unless a showing is given `beside`: Import
// passes none and opens back where it was left; the very first showing picks
// voe_platform_folder_home, else voe_platform_path_absolute(".").
//
// A SHOWING BESIDE A FOLDER STARTS IN ITS PARENT WITH ITS ROW CHOSEN (0343),
// so Open lands on the project in hand and Open again reopens it with no
// click. A folder gone, or a parent that will not list, falls back to home as
// a first showing does; any navigation clears the choice.
//
// A LISTING THAT FAILS CHANGES NOTHING. voe_editor_browser_enter and
// voe_editor_browser_up build the candidate folder, list it into a scratch
// arena of their own, and only touch `arena` — clearing it and copying the new
// rows in — once that listing has already succeeded. So a folder that stops
// being readable between one frame and the next (removed, permissions
// changed) is reported through `why` and leaves the browser exactly where it
// was, never with a folder it could not actually list.
//
// EVERY ROW IS A FOLDER, NEVER A HIDDEN ONE, and a folder row is marked when
// `<row>/project.voe3d` exists, the only thing that tells a project from a
// folder of folders. IMPORT ALONE THEN LISTS THE `.glb` FILES (any case),
// marked as files, because a model is the one file anything here picks:
// Open and Save pick a folder, so a file row there would be a dead end.
//
// THIS FILE CARRIES OUT NO COMMAND OF ITS OWN, exactly as topbar.h's buttons
// do not: voe_editor_browser_clicks_read says what fired — a row entered, Up,
// Confirm (on `target`), Cancel, the name field's own Enter or
// Make folder (MAKE_FOLDER), a file row (IMPORT), or Escape read as a
// Cancel — and what a click
// MEANS, in particular what Confirm does to the project being worked on, is
// session.h's (voe_editor_session_browser_do). Entering a row, going up and
// making a folder are the exception: none of the three changes anything
// about a project, so this file carries all three out itself
// (voe_editor_browser_enter, _up and _make_folder), given the notice to
// write into on failure.
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

// `chosen` when no row is.
#define VOE_EDITOR_BROWSER_NO_ROW UINT32_MAX

// Which the browser is for: a project to open, a folder to save into, or a
// `.glb` to import (folders then `.glb` files, no Confirm).
typedef enum {
	VOE_EDITOR_BROWSER_OPEN,
	VOE_EDITOR_BROWSER_SAVE,
	VOE_EDITOR_BROWSER_IMPORT,
} voe_editor_browser_mode;

// One listed row: its name (not its full path, into the browser's own arena),
// the button drawn for it, whether `<name>/project.voe3d` exists, and whether
// it is a `.glb` file rather than a folder (IMPORT mode only).
typedef struct {
	voe_ui_node node;
	const char *name;
	bool project;
	bool file;
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
	// The row a showing beside a folder chose, an index into `rows`, or
	// VOE_EDITOR_BROWSER_NO_ROW; every successful listing clears it.
	uint32_t chosen;
	// The folder Open's Confirm acts on, absolute, in `arena`: the chosen
	// row's path while one is chosen in OPEN mode, else `folder`.
	const char *target;

	// The three buttons besides the rows, recorded as voe_editor_topbar's
	// are.
	voe_ui_node up_button;
	voe_ui_node confirm_button;
	voe_ui_node cancel_button;

	// SAVE MODE'S NAME ROW (task 14). `name` is the typed folder name,
	// this file's own buffer and not a pointer into `arena` — a field
	// hands its text back as a value (ui/widgets.h) and this is where it
	// is kept between one frame and the next, written back every frame by
	// voe_editor_browser_clicks_read exactly as the inspector writes back
	// a number box's value. Empty ("") outside SAVE mode and the first
	// time a save is shown. `name_field` and `make_button` are
	// VOE_UI_NODE_NONE outside SAVE mode — voe_editor_browser_draw sets
	// both fresh on every call, whatever the mode, so neither is ever a
	// stale node from a showing in the other mode. `focus_name` is set by
	// voe_editor_browser_show on a SAVE showing and consumed by the next
	// voe_editor_browser_draw, which is what takes the keyboard to the
	// name box on the frame the browser opens for a save with no click
	// needed first.
	char name[VOE_UI_FIELD_CAPACITY + 1];
	bool focus_name;
	voe_ui_node name_field;
	voe_ui_node make_button;
} voe_editor_browser;

// Shows browser in mode. `beside` is an absolute folder or NULL, copied and
// never kept. Given, and its parent lists with a row of its name, the browser
// starts in that parent with the row chosen (`target` that row's path in OPEN
// mode). Given but either fails, it starts at the home folder, else the
// current one, with nothing chosen. NULL keeps its folder if one is already
// set, else starts as the given-but-failed case does. The folder is listed
// again in mode, since IMPORT's rows are not OPEN's. A start that fails to
// list is reported through why and the browser still shows, holding whatever
// folder and rows it had before (nothing, on a first showing).
//
// A SAVE SHOWING ALSO ARMS `focus_name`, so the very next
// voe_editor_browser_draw takes the keyboard to the name box — see the
// struct above. `name` itself is untouched: it keeps whatever a previous
// SAVE showing left in it, the same "for the session" the folder already is.
void voe_editor_browser_show(voe_editor_browser *browser,
			     voe_editor_browser_mode mode, const char *beside,
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
// row (marked rows read " — project", file rows " — file", the chosen one a
// choice row, inverted), then — IN SAVE
// MODE ONLY — a name row holding the field and a Make folder button, then
// Confirm ("Open" in OPEN mode, "Save here" in SAVE mode, not drawn in IMPORT
// mode) and Cancel. Records every button into
// browser, read back by voe_editor_browser_clicks_read; outside SAVE mode
// `name_field` and `make_button` are set to VOE_UI_NODE_NONE, fresh every
// call. Consumes `focus_name` when it is set, taking the keyboard to the
// name field on the one frame that asked for it.
void voe_editor_browser_draw(voe_ui_context *ui, voe_editor_browser *browser,
			     float top, voe_math_float2 size);

typedef enum {
	VOE_EDITOR_BROWSER_NONE,
	VOE_EDITOR_BROWSER_ENTERED,
	VOE_EDITOR_BROWSER_UP,
	VOE_EDITOR_BROWSER_CONFIRM,
	VOE_EDITOR_BROWSER_CANCEL,
	VOE_EDITOR_BROWSER_MAKE_FOLDER,
	VOE_EDITOR_BROWSER_IMPORT_FILE,
} voe_editor_browser_action;

// What fired this frame. `name` is the row entered (VOE_EDITOR_BROWSER_ENTERED,
// a pointer into browser's own arena — read it before the next navigation
// clears that arena), the pressed file's absolute path
// (VOE_EDITOR_BROWSER_IMPORT_FILE, in that same arena) or the typed name to
// make a folder from
// (VOE_EDITOR_BROWSER_MAKE_FOLDER, a pointer into browser->name — read it
// before calling voe_editor_browser_make_folder, which may clear that same
// buffer on success). NULL for every other action. Confirm names no folder of
// its own: browser->target already is the one Open acts on.
typedef struct {
	voe_editor_browser_action action;
	const char *name;
} voe_editor_browser_result;

// Which button, row or key fired this frame, or VOE_EDITOR_BROWSER_NONE when
// none did. escape is this frame's Escape key edge, read as a Cancel the same
// as the Cancel button — the caller's, since a window is not this folder's to
// ask (ADR-0141 point 4). Called after voe_ui_frame_end and before the
// frame's arena is rewound, the one window in which a widget will answer.
//
// IN SAVE MODE THIS ALSO WRITES browser->name BACK, WHATEVER ELSE FIRED — the
// name field's own text after this frame's typing, read once here and kept
// for the next call to voe_editor_browser_draw, exactly as the inspector
// writes back a number box's value (inspector.c). That is why `browser` is
// not const here, unlike every other read-back in this program that takes
// the thing it read from by value. VOE_EDITOR_BROWSER_MAKE_FOLDER is reported
// for the name field's own Enter or for Make folder firing, whichever this
// frame did.
voe_editor_browser_result
voe_editor_browser_clicks_read(const voe_ui_context *ui,
			       voe_editor_browser *browser, bool escape);

// Carries out what a MAKE_FOLDER result asked for: refuses name — empty,
// ".", "..", starting with '.', or holding '/' or '\\' — with a notice in
// why and does nothing else. Otherwise calls
// voe_platform_folder_create(join(browser->folder, name)); a failure is a
// notice from the report and browser is untouched; success clears
// browser->name and enters the new folder (voe_editor_browser_enter),
// exactly as picking an existing row would. Asserts if browser has no
// folder yet.
void voe_editor_browser_make_folder(voe_editor_browser *browser,
				    const char *name, voe_editor_notice *why);

// Whether name ends `.glb` in any case: the model file IMPORT lists and the
// Assets panel marks.
bool voe_editor_browser_names_model(const char *name);

// Frees the browser's own arena, when it ever made one. Called once, at
// shutdown.
void voe_editor_browser_destroy(voe_editor_browser *browser);
