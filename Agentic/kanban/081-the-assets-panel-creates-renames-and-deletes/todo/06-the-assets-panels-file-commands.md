# 06 — The Assets panel's file commands
folder: editor
after: 02, 03, 05
decisions: 0168, 0377, 0378

## Change
The commands the panel's menu, keys and drag will call, each checked first and changing nothing when
refused, the refusal said in the session's notice (0378 points 2, 3, 5, 7). No interface here.

New `editor/src/assets_manage.h` and `editor/src/assets_manage.c`. Every path is relative to
`Assets/`, `/` between, as `voe_editor_assets.shown` (assets_panel.h) is; every call takes the session
(session.h), the scene (scene.h), the undo line (undo.h) and a scratch arena, and returns bool.
- `voe_editor_assets_folder_make(..., const char *folder, const char *name)` — one folder made with
  `voe_platform_folder_create`.
- `voe_editor_assets_move(..., const char *from, const char *to)` — a rename when the folder is the
  same, else a move: checked with `voe_editor_assets_follow_check` (assets_walk.h), moved with
  `voe_platform_file_move` (platform/file.h), the files followed with `voe_editor_assets_follow_write`,
  then the open scene followed in memory: `voe_editor_project_scene_text` (project.h), then
  `voe_authoring_paths_follow`, then `voe_editor_project_scene_set`, the selection re-found by authored
  id the way `voe_editor_undo_take` does it (undo.c), the unsaved flag left as it was, then
  `voe_editor_undo_forget`. A path that changed nothing in memory skips the read back.
- `voe_editor_assets_duplicate(..., const char *path)` — a file read and written beside itself as
  `<stem> 2<ext>`, the next free number up to 99 (platform/file.h); a folder refused.
- `voe_editor_assets_trash(..., const char *path)` — `voe_platform_trash` (platform/trash.h);
  UNSUPPORTED said as the file being on another drive than the trash.
- One check every call shares: a name empty, beginning with `.`, or holding `/`, `\` or `"`; a name taken
  in the target folder, by a file or a folder (platform/folder.h's listing); a folder moved into itself or
  below itself; any call while a prefab is open (project.h). Each refusal has its own notice line.
- Each success marks the panel to list again at once (assets_panel.h).
- Header points: why a refusal changes nothing; why the folder is made only now (0378 point 4); why the
  undo line is forgotten; the order of check, move, write, memory.

Add both files' entries to `editor/src/src.md`.

## Done when
`grep -c 'voe_editor_assets_folder_make\|voe_editor_assets_move\|voe_editor_assets_duplicate\|voe_editor_assets_trash' editor/src/assets_manage.h`
prints 4 or more, and the folder's check passes.
