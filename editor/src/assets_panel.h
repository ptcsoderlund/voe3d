// The Assets panel (0270, 0277 point 6): the project's `<project>/Assets/`
// folder as rows, folders first and then files, each by name, hidden ones left
// out. A folder row is entered, Up goes back a level, and the listing follows
// the project in place. main.c updates it once a frame; dock.c draws it under
// the Scene list and interface.c reads its clicks beside the other panels':
//
//     voe_editor_assets_update(&scene.assets, session.project->folder, now);
//     voe_editor_assets_draw(ui, &scene->assets);          // in the dock
//     voe_editor_assets_clicks_read(ui, &scene->assets);   // after frame end
//
// ITS OWN ARENA, cleared at every listing: the shown folder, the project it
// was listed for and every row name are copied in, so nothing points into the
// session's project, which New and Open replace. Made on the first update and
// freed by voe_editor_assets_destroy at shutdown.
//
// ONCE A SECOND it lists again, so a file copied in from outside the editor
// shows up without a click, and it is not a folder read every frame. A
// different project folder lists at once, back at `Assets/`.
//
// NEVER ABOVE `Assets/`. `shown` is relative to it and Up is not drawn at it:
// the panel is the project's files, and the rest of the disk is the
// browser's (browser.h).
//
// A MODEL ROW is a file whose name ends `.glb`, in any case: the one kind a
// thing can draw in 0.2 (0270). It is drawn marked, as the Scene list marks
// its selected row; other files are only listed.
//
// A FAILED LISTING KEEPS THE OLD ROWS for the same project, as browser.h's
// does, and says why on stderr through platform/folder.h. `Assets/` is looked
// for in the project's own listing first, so a project without one, or an
// untitled project with no folder, is no rows and `missing`, shown as a line,
// with nothing on stderr. Constraint: a shown subfolder removed from outside
// reports on stderr once a second until Up is pressed.
//
// THE ROWS SCROLL IN THE LEAF'S OWN SCROLL AREA (dock.c): every panel that is
// not a picture already is one, and a second inside it would have nothing to
// grow against.
#pragma once

#include "browser.h"

#include <base/arena.h>

#include <ui/widgets.h>

#include <stdbool.h>
#include <stdint.h>

// One listed entry: its name in the panel's arena, whether it is a folder,
// whether it is a model, and the row drawn for it.
typedef struct {
	voe_ui_node node;
	const char *name;
	bool folder;
	bool model;
} voe_editor_assets_row;

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
} voe_editor_assets;

// Lists again when `project_folder` (NULL for untitled) differs from the one
// listed, back at `Assets/`, or a second of `now`, the frame clock in seconds,
// has passed since the last listing.
void voe_editor_assets_update(voe_editor_assets *assets,
			      const char *project_folder, double now);

// Up (not at `Assets/`), the shown folder's path, then one row per entry, or
// the line saying there is no `Assets/`. Records every node for the read.
void voe_editor_assets_draw(voe_ui_context *ui, voe_editor_assets *assets);

// After voe_ui_frame_end: a folder row fired is entered, Up fired goes up a
// level; either lists at once.
void voe_editor_assets_clicks_read(const voe_ui_context *ui,
				   voe_editor_assets *assets);

// Frees the arena, when one was made. Called once, at shutdown.
void voe_editor_assets_destroy(voe_editor_assets *assets);
