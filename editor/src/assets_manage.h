// The Assets panel's file commands: make a folder, rename or move, duplicate,
// and send to the trash (ADR-0377 point 3, 0378). The panel's menu, keys and
// drag call them; nothing here draws.
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
// opening, so a cancelled or refused name leaves nothing behind.
//
// A MOVE GOES CHECK, MOVE, WRITE, MEMORY. The project's texts are followed in
// scratch and the open scene's text too, nothing written; then the move; then
// the texts written (a moved folder's own prefabs are read where they now are);
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
// Trash is the home trash only (platform/trash.h).
#pragma once

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

// Renames or moves `from` to `to`, following every path that names it on disk
// and in the open scene, then forgets the undo line. `from` equal to `to` does
// nothing and answers true.
[[nodiscard]] bool voe_editor_assets_move(voe_editor_session *session,
					  voe_editor_scene *scene, voe_editor_undo *undo,
					  voe_base_arena *scratch, const char *from,
					  const char *to);

// Copies the file at `path` beside itself as `<stem> 2<ext>`, the next free
// number up to 99. A folder is refused.
[[nodiscard]] bool voe_editor_assets_duplicate(voe_editor_session *session,
					       voe_editor_scene *scene,
					       voe_editor_undo *undo,
					       voe_base_arena *scratch, const char *path);

// Sends the file or folder at `path` to the desktop's home trash.
[[nodiscard]] bool voe_editor_assets_trash(voe_editor_session *session,
					   voe_editor_scene *scene, voe_editor_undo *undo,
					   voe_base_arena *scratch, const char *path);
