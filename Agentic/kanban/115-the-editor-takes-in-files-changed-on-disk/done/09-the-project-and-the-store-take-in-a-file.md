# 09 — The project and the store take in what a file says
folder: editor
after: 08
decisions: 0168, 0204, 0470, 0506, 0509

## Change
Add calls that put what a file on disk says into the part of the editor that owns it. None of them
writes anything.

- `editor/src/project.h`, `editor/src/project.c` (742 lines; keep it under 800):
  - `bool voe_editor_project_scene_take(voe_editor_project *project, const char *text, size_t size, voe_base_arena *scratch, voe_editor_notice *why)`,
    for the open scene or the open prefab:
    - First it runs `voe_authoring_scene_check` against the project's world. Then it sets the
      text the way `_scene_set` does, and sets `unsaved` to false.
    - Refused, it returns false and sets `why` from the kept report naming the file. The world
      stays as it was: the check runs first, so only running out of room can half-load it, as
      `_scene_set` says.
  - `void voe_editor_project_file_take(voe_editor_project *project, const voe_authoring_project *file)`:
    - The project file's values become `file`'s. They are copied so they outlive `file`, and
      repeated takes must not grow an arena without bound.
    - It writes nothing and leaves `unsaved` and undo untouched.
- `editor/src/models.h`, `editor/src/models.c` (725 lines):
  - `void voe_editor_models_landscape_take(voe_editor_models *models, const char *path, const voe_assets_landscape *landscape, bool edited)`:
    - It copies `landscape` into the models' own memory. `voe_assets_landscape_resample` at the
      landscape's own cells makes an exact copy.
    - The copy is loaded over `path`'s entry at the next update, between frames, the way
      `_landscape_shape`'s re-read is. Then it calls `voe_3d_models_landscape_changed` when
      `edited` is true.
    - A newer take of the same path replaces a pending one. For a path the store does not hold, it
      does nothing.
  - `const voe_assets_landscape *voe_editor_models_landscape_held(const voe_editor_models *models, const char *path)`:
    the store's landscape, or NULL when it is not loaded.
  - `bool voe_editor_models_landscape_edited(const voe_editor_models *models, const char *path)`:
    that landscape's `edited` flag.
- Headers of the five calls: what each takes in, and that nothing is written or cooked (0470).

## Meaning
- Scout: unsaved is tracked per file in part. The scene and an open prefab share `unsaved`, the
  set-aside level has `level_unsaved`, and each landscape has the store's `edited` flag.

## Done when
- `grep -q voe_editor_project_scene_take editor/src/project.h`,
  `grep -q voe_editor_project_file_take editor/src/project.h` and
  `grep -c 'voe_editor_models_landscape_\(take\|held\|edited\)' editor/src/models.h` (prints 3 or
  more) all succeed, and `project.c` is under 800 lines.
- `d=$(mktemp -d) && XDG_CONFIG_HOME=$d build/debug/editor/voe_editor --capture $d/a.png && test -s $d/a.png`
  exits 0.
