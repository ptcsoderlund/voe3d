# 33 — Material edits are saved at rest and undone
folder: editor
after: 32
decisions: 0168, 0204, 0379, 0399
read: feature.md

## Change
0399 point 9: an edit to the open material is written to its file when the editor comes to rest, and
is one undo step carried beside the scene text, as a sculpting stroke is.

- New `editor/src/material_steps.h` / `.c`, shaped as `strokes.h` / `.c` (read them):
  `voe_editor_material_step` holding the path and the `voe_assets_material_file` before and after, its
  own malloced memory; `_new`, `_destroy`, and `void voe_editor_material_step_apply(const
  voe_editor_material_step *step, voe_editor_models *models, voe_editor_scene *scene, const char
  *folder, voe_base_arena *scratch, bool forward)`: the values taken into the table row, written to
  `<folder>/<path>` (`voe_assets_material_write`, written through `platform` as a landscape's save
  writes), the store told through `voe_editor_models_material_changed` with `maps` true when a map
  path differs, and `scene->material` and its `material_before` taken too when the step's path is
  the open one.
- `editor/src/undo.h` / `undo.c` — a state may carry a material step as it carries a stroke:
  `voe_editor_undo_material(...)` pushes a state whose text equals the one before; a step over it
  applies `before` going back and `after` coming forward; a dropped state frees it. Header paragraph
  "OR A STEP IS A MATERIAL EDIT".
- `editor/src/scene.h` / `scene.c` — `voe_assets_material_file material_before`, set when a material is
  opened (card 29's open in `interface_assets.c`).
- Where the undo line is settled at rest (`editor/src/frame_commands.c`'s history step; its header
  says so): at rest, with `material_open` set and `scene->material` differing from
  `material_before`, the file is written (a refused write said in the session's notice), a step
  pushed, `material_before` taken from `material`. The project is not marked unsaved.

Update `editor/src/src.md` for the new and changed files.

## Done when
`grep -n 'voe_editor_undo_material' editor/src/*.c` finds the push and the editor builds. Human, the
feature's How to test 1–7 in one sitting: import the dirt set; Create → Material Dirt; drop the three
maps; give Dirt to a cube (bumps in the light); Repeat 1 → 4 tiles four times at once; roughness and
colour change while dragging, Ctrl+Z goes back; Duplicate Dirt, tint the copy, give it to a second
cube, each keeps its own; rename Dirt, the first cube still shows it; Save, close, reopen, both as
left; Play shows the same.
