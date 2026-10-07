// The Assets panel (0270, 0277 point 6): the project's `<project>/Assets/`
// folder as rows, folders first and then files, each by name, hidden ones left
// out. A folder row is entered, Up goes back a level, Import copies a `.glb`
// chosen in the browser's IMPORT mode into the shown folder (0277 point 7),
// and the listing follows the project in place. main.c updates it once a
// frame; dock.c draws it and interface.c reads it after the frame ends:
//
//     voe_editor_assets_update(&scene.assets, session.project->folder, now);
//     voe_editor_assets_draw(ui, &scene->assets);          // in the dock
//     voe_editor_assets_clicks_read(ui, &scene->assets, down, over);
//
// ITS OWN ARENA, cleared at every listing, holds the shown folder, the project
// and every row name, so nothing points into the session's project, which New
// and Open replace. Made on the first update, freed by voe_editor_assets_destroy.
//
// ONCE A SECOND it lists again, so a file copied in from outside shows up; a
// different project folder lists at once, back at `Assets/`. NEVER ABOVE
// `Assets/`: `shown` is relative to it and Up is not drawn there; the rest of
// the disk is the browser's (browser.h).
//
// EVERY ROW IS DRAWN ALIKE, a choice in the theme's text colour, its kind told
// by the name's ending alone (0194). A MODEL (`.glb`), PREFAB (`.prefab`) or
// PICTURE (`.png`, `.jpg`, `.jpeg`) row, in any case, is held so assets_drag.h
// places it or makes it a texture (0270, 0283, 0298). A prefab row pressed and
// released on the row fires: its project-relative path, `/` between, is left
// in `opened` for the caller to read and clear; one too long for it is not.
//
// THE SELECTED ROW (0378 point 6) is the last row pressed, drawn selected,
// kept across a listing while its name is still there, cleared on entering a
// folder or Up. A row pressed also takes `keyboard`, F2 and Delete being the
// panel's then (shortcuts.h); a primary press outside the panel gives it back.
//
// NAMING IN PLACE (0378 point 4): rename_begin draws the selected row, and
// folder_begin a pending row first among the folders, as a field focused with
// its name selected. Enter or a press elsewhere leaves a `request` the caller
// carries out through assets_manage.h and clears; Escape leaves none. Either
// ends the naming, as do entering a folder, Up, or a listing without the row.
// delete_begin leaves the selected row's path in `deleting`, which the caller
// asks about (assets_ask.h) and clears.
//
// A FAILED LISTING KEEPS THE OLD ROWS for the same project, as browser.h's
// does, and says why on stderr through platform/folder.h. A project without
// `Assets/`, or untitled, is no rows and `missing`, shown as a line, nothing
// on stderr. Constraints: a shown subfolder removed from outside reports on
// stderr once a second until Up; naming does not begin in a shown folder too
// long for VOE_EDITOR_ASSETS_PATH with a whole field's name after it, which a
// longer buffer would lift.
//
// THE ROWS SCROLL IN THE LEAF'S OWN SCROLL AREA (dock.c), and the empty space
// under them is a node of its own, so a read can test a press there.
#pragma once

#include "browser.h"

#include <base/arena.h>

#include <scene/prefab_component.h>

#include <ui/widgets.h>

#include <stdbool.h>
#include <stdint.h>

// One listed entry: its name in the panel's arena, whether it is a folder,
// whether it is a model, a prefab or a picture, and the row drawn for it.
typedef struct {
	voe_ui_node node;
	const char *name;
	bool folder;
	bool model;
	bool prefab;
	bool picture;
} voe_editor_assets_row;

// A request's paths, `/` between: room for the shown folder and a field's name.
#define VOE_EDITOR_ASSETS_PATH 1024

// The naming in progress, and the kind of a request it left.
typedef enum {
	VOE_EDITOR_ASSETS_NAMING_NONE = 0,
	VOE_EDITOR_ASSETS_NAMING_RENAME,
	VOE_EDITOR_ASSETS_NAMING_FOLDER,
} voe_editor_assets_naming;

// What a committed field asks for, paths relative to `Assets/` as
// assets_manage.h takes them: a rename's `from` and `to`, the shown folder
// joined with the row's and the typed name, or a folder's `folder` and `name`.
// `name` is the typed text as it is, so the caller can refuse a `/` in it.
typedef struct {
	voe_editor_assets_naming kind;
	char folder[VOE_EDITOR_ASSETS_PATH];
	char from[VOE_EDITOR_ASSETS_PATH];
	char to[VOE_EDITOR_ASSETS_PATH];
	char name[VOE_UI_FIELD_CAPACITY + 1];
} voe_editor_assets_request;

// The panel's whole state. Zeroed is a panel never listed.
typedef struct {
	voe_base_arena *arena;
	// The project folder listed, absolute, NULL for untitled; `shown`
	// relative to its `Assets/`, "" at `Assets/` itself; `path` the two as
	// the label reads, `Assets/<shown>`. All three in `arena`.
	const char *project;
	const char *shown;
	const char *path;
	// Whether anything was ever listed, and the frame clock then.
	bool listed_once;
	double listed;
	// No `Assets/` folder, or no project folder at all.
	bool missing;
	voe_editor_assets_row rows[VOE_EDITOR_BROWSER_ROWS];
	uint32_t row_count;
	voe_ui_node up_button;
	voe_ui_node import_button;
	// The model, prefab or picture row the pointer went down on and still
	// holds, as the last read found it, NULL for none: its name in `arena`,
	// valid until the next listing, and whether it is a prefab or a
	// picture. What assets_drag.h starts a drag from.
	const char *held;
	bool held_prefab;
	bool held_picture;
	// The prefab row fired at the last read, `Assets/...` under the
	// project, "" for none. The caller clears it once it has opened it.
	char opened[VOE_SCENE_PREFAB_PATH];
	// The selected row's name, one of the rows' own in `arena`, NULL for
	// none; and whether F2 and Delete are the panel's.
	const char *selected;
	bool keyboard;
	// The naming in progress, its field focused at the next draw while
	// `naming_focus`, the field drawn, and the space under the rows.
	voe_editor_assets_naming naming;
	bool naming_focus;
	voe_ui_node naming_field;
	voe_ui_node empty;
	// Left by a committed field; the caller carries it out and sets `kind`
	// back to NONE.
	voe_editor_assets_request request;
	// Left by delete_begin: the selected row's path relative to `Assets/`,
	// `/` between, "" for none. The caller asks about it and clears it.
	char deleting[VOE_EDITOR_ASSETS_PATH];
} voe_editor_assets;

// Lists again when `project_folder` (NULL for untitled) differs from the one
// listed, back at `Assets/`, or a second of `now`, the frame clock in seconds,
// has passed since the last listing.
void voe_editor_assets_update(voe_editor_assets *assets,
			      const char *project_folder, double now);

// The next update lists again at once, in the same shown folder: a file
// command (assets_manage.h) changed what is there.
void voe_editor_assets_list_due(voe_editor_assets *assets);

// Up (not at `Assets/`) beside Import (only when the project has a folder),
// the shown folder's path, then a new folder's field, one row per entry, the
// renamed one a field, and the space under them; or the line saying there is
// no `Assets/`. Records every node for the read.
void voe_editor_assets_draw(voe_ui_context *ui, voe_editor_assets *assets);

// After voe_ui_frame_end: the naming's field read into `request`, then a
// folder row fired is entered, Up fired goes up a level; either lists at once.
// A row held is selected and takes `keyboard`; `pointer_down` while the
// pointer is not `over` the panel gives it back. `held` is set to the model,
// prefab or picture row held, if any, and `opened` to a prefab row fired.
// True when Import fired, which the caller answers by showing the browser in
// IMPORT mode.
bool voe_editor_assets_clicks_read(const voe_ui_context *ui,
				   voe_editor_assets *assets, bool pointer_down,
				   bool over);

// The selected row drawn as a field holding its name from the next draw;
// nothing without a selected row.
void voe_editor_assets_rename_begin(voe_editor_assets *assets);

// A pending "New folder" row drawn as a field first among the folders.
void voe_editor_assets_folder_begin(voe_editor_assets *assets);

// The selected row's path left in `deleting` for the caller to ask about;
// nothing without a selected row or when the path does not fit.
void voe_editor_assets_delete_begin(voe_editor_assets *assets);

// Copies the file at `source` into the shown folder under its own name, read
// whole and written atomically over any file of that name, making `Assets/`
// first when it is missing, then lists again. A failure is said in why,
// naming the file, and changes nothing else.
void voe_editor_assets_import(voe_editor_assets *assets,
			      const char *project_folder, const char *source,
			      voe_editor_notice *why);

// Frees the arena, when one was made. Called once, at shutdown.
void voe_editor_assets_destroy(voe_editor_assets *assets);
