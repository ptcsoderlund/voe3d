// The Assets panel's file commands: make a folder, a landscape or a material,
// rename or move, duplicate, and send to the trash (ADR-0377 point 3, 0378).
// The panel's menu, keys and drag call them; nothing here draws.
//
//     if (voe_editor_assets_move(session, scene, undo, scratch, "Rocks/a.glb",
//                                "Stone/a.glb"))
//             ...                     // moved, followed, panel lists again
//
// PATHS are relative to `Assets/`, `/` between, as voe_editor_assets.shown is;
// "" is `Assets/` itself. Every call answers true on success; false means
// refused or failed, with the reason in session->notice.
//
// A REFUSAL CHANGES NOTHING. Every check runs before the first byte on disk
// moves: the project has a folder; no prefab is open, since the level set aside
// as text would not follow (0378 point 3); a new name is not empty, does not
// begin with `.` and holds no `/`, `\` or `"`; it is not taken in its folder by
// a file or a folder (a move never replaces); a folder is not moved into or
// below itself; and every followed path still fits its field. So a person can
// try again with nothing to undo by hand.
//
// A FOLDER IS MADE ONLY WHEN ITS NAME IS COMMITTED (0378 point 4): Create ->
// Folder's field calls folder_make on Enter or a press elsewhere, never on
// opening, so a cancelled or refused name leaves nothing behind. Create ->
// Landscape's field calls landscape_make the same way: `.landscape` appended
// unless the name ends so in any case, the name refused as a folder's, then a
// flat grid of 256 m and 512 cells written (0379 point 1); its caller lists.
// Create -> Material's calls material_make so too, `.material` appended and
// voe_assets_material_default()'s text written (0399 point 1); its caller lists.
//
// THE MATERIALS' TABLE IS READ AGAIN (models.h) after every successful
// material make, move, duplicate and trash (0399 point 7).
//
// A MOVE GOES CHECK, MOVE, WRITE, MEMORY. The project's texts are followed in
// scratch and the open scene's text too, nothing written; then the move, and
// the model store's entries renamed so a landscape keeps its unsaved heights
// (0379 point 6); then the texts written (a moved folder's own prefabs are read where they now are);
// then the open scene read back from its followed text, its selection re-found
// by authored id and its unsaved flag as it was. A write that fails after the
// move is said in the notice; the move stands, as it must.
//
// THE UNDO LINE IS FORGOTTEN after a move: its states are scene texts naming
// the old paths, and stepping to one would break them again (0378 point 2).
//
// EVERY SUCCESS MARKS THE PANEL to list again at once (assets_panel.h).
//
// CONSTRAINTS: a move's name is `to`'s last segment, so a Rename's caller
// joins the shown folder and the typed name only after refusing a typed `/`.
// Duplicate reads the file whole and refuses an empty one; names stop at 99.
// Trash is the desktop's, the home trash or the Recycle Bin (platform/trash.h).
#pragma once

#include "models.h"
#include "scene.h"
#include "session.h"
#include "undo.h"

#include <base/arena.h>

#include <stdbool.h>

// Makes `<folder>/<name>`, one level, with voe_platform_folder_create.
[[nodiscard]] bool voe_editor_assets_folder_make(voe_editor_session *session,
						 voe_editor_scene *scene,
						 voe_editor_undo *undo,
						 voe_base_arena *scratch,
						 const char *folder, const char *name);

// Makes `<folder>/<name>.landscape`, a flat one; the panel is not marked.
[[nodiscard]] bool voe_editor_assets_landscape_make(voe_editor_session *session,
						    voe_base_arena *scratch,
						    const char *folder, const char *name);

// Makes `<folder>/<name>.material` holding the defaults; the panel is not marked.
[[nodiscard]] bool voe_editor_assets_material_make(voe_editor_session *session,
						   voe_editor_models *models,
						   voe_base_arena *scratch,
						   const char *folder, const char *name);

// Renames or moves `from` to `to`, following every path that names it on disk,
// in the open scene and in `models`, then forgets the undo line. `from` equal
// to `to` does nothing and answers true.
[[nodiscard]] bool voe_editor_assets_move(voe_editor_session *session,
					  voe_editor_scene *scene, voe_editor_undo *undo,
					  voe_editor_models *models,
					  voe_base_arena *scratch, const char *from,
					  const char *to);

// Copies the file at `path` beside itself as `<stem> 2<ext>`, the next free
// number up to 99. A folder is refused.
[[nodiscard]] bool voe_editor_assets_duplicate(voe_editor_session *session,
					       voe_editor_scene *scene,
					       voe_editor_undo *undo,
					       voe_editor_models *models,
					       voe_base_arena *scratch, const char *path);

// Sends the file or folder at `path` to the desktop's trash, the home trash or
// the Recycle Bin (platform/trash.h).
[[nodiscard]] bool voe_editor_assets_trash(voe_editor_session *session,
					   voe_editor_scene *scene, voe_editor_undo *undo,
					   voe_editor_models *models,
					   voe_base_arena *scratch, const char *path);
