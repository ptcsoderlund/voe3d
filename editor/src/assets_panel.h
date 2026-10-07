// The Assets panel (0270, 0277 point 6): the project's `<project>/Assets/`
// folder as rows, folders first and then files, each by name, hidden ones left
// out. A folder row is entered, Up goes back a level, Import copies a `.glb`
// chosen in the browser's IMPORT mode into the shown folder (0277 point 7),
// and the listing follows the project in place. main.c updates it once a
// frame; dock.c draws it under
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
// EVERY ROW IS DRAWN ALIKE, an unselected choice in the theme's text colour:
// the kind is shown by the name's ending alone, never by colour (0194).
//
// A MODEL ROW is a file whose name ends `.glb`, in any case: the one kind a
// thing can draw in 0.2 (0270). It is held so assets_drag.h places it; other
// files are only listed.
//
// A PREFAB ROW is a file whose name ends `.prefab`, in any case (0283): held
// as a model row is, so assets_drag.h places it. Pressed and released on the
// row, which a drag to a view never is, it fires: the read leaves its
// project-relative path, `/` between, in `opened`, a field the caller reads
// and clears; a path too long for it is not reported.
//
// A PICTURE ROW is a file whose name ends `.png`, `.jpg` or `.jpeg`, in any
// case (0298 point 5): held as a model row is, so assets_drag.h can make it
// an emitter's texture.
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
// the shown folder's path, then one row per entry, or the line saying there
// is no `Assets/`. Records every node for the read.
void voe_editor_assets_draw(voe_ui_context *ui, voe_editor_assets *assets);

// After voe_ui_frame_end: a folder row fired is entered, Up fired goes up a
// level; either lists at once. `held` is set to the model, prefab or picture
// row held, if any, and `opened` to a prefab row fired. True when Import fired, which
// the caller answers by showing the browser in IMPORT mode.
bool voe_editor_assets_clicks_read(const voe_ui_context *ui,
				   voe_editor_assets *assets);

// Copies the file at `source` into the shown folder under its own name, read
// whole and written atomically over any file of that name, making `Assets/`
// first when it is missing, then lists again. A failure is said in why,
// naming the file, and changes nothing else.
void voe_editor_assets_import(voe_editor_assets *assets,
			      const char *project_folder, const char *source,
			      voe_editor_notice *why);

// Frees the arena, when one was made. Called once, at shutdown.
void voe_editor_assets_destroy(voe_editor_assets *assets);
