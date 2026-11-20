# 10 — A file step and a landscape step
folder: editor
after: 09
decisions: 0168, 0204, 0437, 0506, 0509

## Change
Make new files `editor/src/file_steps.h` and `editor/src/file_steps.c`. First read the headers of
`strokes.h` and `material_steps.h`: these steps have the same shape, and are malloced under rule
11's exception.

- `bool voe_editor_file_take(voe_editor_project *project, voe_editor_models *models, const char *path, const char *bytes, size_t size, voe_base_arena *scratch, voe_editor_notice *why)`
  - It takes the file at the project-relative `path` into what the editor holds. Its bytes are on
    disk and also given. It writes nothing. By kind:
    - `.material`: `voe_assets_material_read_strict` refuses a wrong one, then
      `voe_editor_models_materials_read`.
    - `.parts`: `voe_assets_parts_read` refuses a wrong one (0437 point 3 stands), then
      `voe_editor_models_materials_read`.
    - A `.prefab` that is not open: first `voe_authoring_scene_check`. Then the world's own text
      (`voe_editor_project_scene_text`) is set again with `_scene_set`, so its copies expand from
      the new file. `unsaved` is put back as it was.
    - `project.voe3d`: `voe_authoring_project_read_strict`, then
      `voe_editor_project_file_take`.
  - Refused, it returns false, and `why` names the file, line, key and why from the kept report.
    Clear the report first.
- `voe_editor_file_step` holds a path and its own copies of the before and after bytes.
  - `_new(path, before, before_size, after, after_size)` and `_destroy`.
  - `_apply(step, project, models, watch, scratch, why, bool forward)` writes that side's bytes to
    `<folder>/<path>`, tells the watch, then calls `voe_editor_file_take`. The editor writes these
    kinds at once, so a step over them writes too (0509 point 2).
- `voe_editor_landscape_step` holds a path and whole landscapes before and after, each copied into
  its own memory (a resample at its own cells).
  - `_new` and `_destroy`.
  - `_apply(step, models, bool forward)` calls `voe_editor_models_landscape_take` with `edited`
    true. A landscape is saved with the project, so a step over it leaves it unsaved, as a stroke
    does.
- Header points:
  - the two steps;
  - why bytes for small files, and whole copies for a landscape;
  - how much memory each holds.
- `editor/src/src.md`: an entry for the new files.

## Meaning
- Scout: materials and the project file are never unsaved; an edit writes them at once.

## Done when
- `grep -q voe_editor_file_take editor/src/file_steps.h` and
  `grep -q voe_editor_landscape_step_apply editor/src/file_steps.h` both exit 0.
- `d=$(mktemp -d) && XDG_CONFIG_HOME=$d build/debug/editor/voe_editor --capture $d/a.png && test -s $d/a.png`
  exits 0.
