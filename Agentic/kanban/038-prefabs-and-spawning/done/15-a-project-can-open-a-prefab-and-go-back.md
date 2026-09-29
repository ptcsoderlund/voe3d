# 15 — A project can open a prefab, save it, and go back to the level
folder: editor
decisions: 0168, 0283, 0164, 0204

## Change
Needs cards 09 and 13. 0283 point 8, the project's share; session and screen are cards 16–17.

- `editor/src/project.h`, `editor/src/project.c`:
  - `voe_editor_project` gains `prefab` (char, `VOE_SCENE_PREFAB_PATH`, "" while the level is
    open) and what is set aside while one is: an arena of its own (NULL when none), the level's
    text and size, and its unsaved flag.
  - `[[nodiscard]] bool voe_editor_project_prefab_open(voe_editor_project *, const char *path,
    voe_editor_notice *why);` — asserts a folder and no prefab open; writes the level's text
    with `voe_editor_project_scene_text` (kept sections included) into the new arena, reads
    `<folder>/<path>` and makes the world hold it through `voe_editor_project_scene_set`; sets
    `prefab`, and `unsaved` false. On any failure the level is put back as it was and why says so.
  - `[[nodiscard]] bool voe_editor_project_prefab_back(voe_editor_project *, voe_editor_notice
    *why);` — reads the set-aside text back through `scene_set` (so every copy is expanded from the
    files as they are now), restores the level's unsaved flag, destroys the arena, clears
    `prefab`.
  - `voe_editor_project_save` with a prefab open writes the world's text to `<folder>/<prefab>`,
    refusing with why a world without exactly one entity with no parent, or one where
    `voe_editor_prefab_refused` (prefabs.h) answers true for that root.
  - `voe_editor_project_destroy` destroys the set-aside arena too.
  - The header: a paragraph on the prefab document (0283 point 8): what is set aside, why as
    text, what Save writes, and that a Refresh's `code_set` keeps both.
- `editor/src/prefab_make.c`: `voe_editor_prefab_make` refuses while a prefab is open.
- `editor/src/assets_drag.c`: a prefab is not placed while a prefab is open (notice).
- `editor/src/src.md`: the changed entries.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0, and
`d=$(mktemp -d) && build/debug/editor/voe_editor examples/tank_game --capture "$d/t.png" && test
-s "$d/t.png"` exits 0. Nothing calls the two new functions until card 16; the human's check is
card 17's.
