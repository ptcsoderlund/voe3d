# 05 — The project's texts are walked for a path
folder: editor
after: 01, 04
decisions: 0168, 0378

## Change
The files on disk that follow a rename or move, and that Delete names as users (0378 point 1).

New `editor/src/assets_walk.h` and `editor/src/assets_walk.c`:
- `VOE_EDITOR_ASSETS_PATH_ROOM` — 128, the room of every path field a scene holds (a model's, a
  prefab's, a texture's, a sound's); a followed path must be shorter.
- The walk: every file under the project folder whose name ends in an extension of one list (`.scene`,
  `.prefab`; a later own kind adds its own), any case, skipping hidden entries and the top-level
  `Build`, `Cache` and `Code` folders; an explicit stack of folders through
  `voe_platform_folder_list` (platform/folder.h), no recursion, a named depth limit past which a
  folder is skipped and said once on stderr. Each file read with `voe_platform_file_read`.
- `voe_editor_assets_users(const char *project, const char *path, voe_base_arena *arena,
  voe_editor_assets_used *out)` — a struct of up to 4 project-relative names of files naming `path`
  (`voe_authoring_paths_named`, authoring/paths.h) and how many in all.
- `[[nodiscard]] bool voe_editor_assets_follow_check(const char *project, const char *from,
  const char *to, voe_base_arena *scratch, voe_editor_notice *why)` — follows every walked file in
  scratch with `voe_authoring_paths_follow` and writes nothing; false with why naming the file when a
  rewritten path would not be shorter than the room, or a file will not read or follow.
- `[[nodiscard]] bool voe_editor_assets_follow_write(...)`, same parameters — follows and writes each
  file that changed with `voe_platform_file_write`; false with why naming the first that would not.
- `from` and `to` are project-relative, `Assets/...`, `/` between; scratch is rewound before return.
- Header points: why a check comes before the move and the write after it (a moved folder's prefabs
  are read where they now are); why a file reads as text, never a world; that C code is never walked
  (0377 point 3).

Add both files' entries to `editor/src/src.md`.

## Done when
`grep -c 'voe_editor_assets_follow_check\|voe_editor_assets_follow_write\|voe_editor_assets_users' editor/src/assets_walk.h`
prints 3 or more, and the folder's check passes.
