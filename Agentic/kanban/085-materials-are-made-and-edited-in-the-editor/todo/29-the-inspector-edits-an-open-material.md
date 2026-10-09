# 29 — The Inspector edits an open material
folder: editor
after: 27
decisions: 0168, 0399

## Change
0399 point 8: a pressed `.material` row opens it in the Inspector; its values change live.

- `editor/src/scene.h` / `scene.c` — `char material_open[VOE_ASSETS_MATERIAL_PATH]` (empty: none) and
  `voe_assets_material_file material`, the shown copy. `voe_editor_scene_select` and the Scene list's
  clicks read clear `material_open`. New `void voe_editor_scene_material_follow(voe_editor_scene
  *scene, const char *from, const char *to)`: the open path follows a move of it or a folder above
  it; `to` NULL (a trash) closes it when it is or is under `from`.
- `editor/src/assets_panel.h` / `.c` — a `.material` row fires into `material_opened` as a
  `.landscape` row fires into `landscape_opened`.
- `editor/src/interface_assets.c` — a fired material row: the selection cleared, `material_open` set
  and `material` copied from `voe_editor_materials_find` (card 25); not found, nothing opens. After
  each successful move or trash, `voe_editor_scene_material_follow`. The same after F2's rename and
  the Delete key's trash in `editor/src/frame_commands.c`.
- New `editor/src/inspector_material.h` / `.c`, shaped as `inspector_sculpt.h` / `.c` (read them):
  the file's name as heading; Lit and Unlit buttons, the chosen one lit; a Colour swatch as a button
  (card 30 opens the picker on it); Roughness and Metal sliders 0–1; Repeat a number box clamped
  0.01–1000; Colour map, Normal map and Roughness map rows, each the map's file name or "None", with ×
  while set. Its records kept for card 32's drop on a map row. The read after the frame changes
  `scene->material` only.
- `editor/src/dock_walk.c` / `inspector.c` — while `material_open` is set the Inspector draws the
  material section in place of the entity's.
- `editor/src/interface_read.c` — after the Inspector's read, when `scene->material` differs from the
  table's row: the row takes it, then new `void voe_editor_models_material_changed(voe_editor_models
  *models, const char *path, bool maps)` in `editor/src/models.h` / `.c`: factors only →
  `voe_3d_models_material_set` (live next frame); a map changed → reloaded by
  `voe_game_models_material_load` at the next update, between frames.

Headers say each point; `editor/src/src.md` gains the new files and changes the others' lines.
Nothing is written to the file yet (card 33).

## Done when
`grep -n 'material_open' editor/src/dock_walk.c editor/src/inspector.c` finds the switch, and the
editor builds. Human: press `Dirt.material` in Assets; the Inspector shows it; dragging Roughness
changes a cube that wears it while dragging (once card 32 can give it).
