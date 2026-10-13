# 36 — Importing a glb writes its materials and parts
folder: editor
after: 35
decisions: 0168, 0403, 0404, 0420

## Change
0403's import and 0420 point 3's first half: a `.glb` imported into Assets brings one `.material` per
glTF material and its `.parts`, written again on every import (How to test 1 and 7).

- `editor/src/model_import.h`, `editor/src/model_import.c` (new, header comments):
  `[[nodiscard]] bool voe_editor_model_import(const char *project_folder, const char *glb,
  voe_base_arena *scratch, voe_editor_notice *why)`, `glb` the `Assets/…` path of a `.glb` already in
  place: reads and parses it (`platform/file.h`, `assets/model.h`); for each glTF material writes
  `<stem>-<name>.material` beside it from `voe_assets_model_material_values`
  (`assets/model_material.h`) through `voe_assets_material_write`, names per 0420 point 3; then
  `<stem>.parts` (`assets/parts.h`) naming each by its `Assets/…` path. A file of a name is
  overwritten; a file with more than VOE_ASSETS_PARTS materials, one that will not parse, or a write
  that fails is said in `why` naming the file, nothing after it written. Header: what is written and
  why it overwrites (0403: the material comes from the `.glb`; PNGs are never touched here).
- `editor/src/assets_panel.c`, `editor/src/assets_panel.h`: `voe_editor_assets_import` calls it after
  a `.glb`'s copy succeeds; its comment says so.
- `editor/src/scene.h`, `editor/src/scene.c`: one call that, after the materials are read again, gives
  the open material its row's values as both `material` and `material_before`, closing it when its row
  is gone, so a re-import is not taken for an Inspector edit and written back.
- `editor/src/session.c` (the browser's IMPORT_FILE case) and `editor/src/assets_manage.c` (each
  `voe_editor_models_materials_read`): after an import, the tables are read again
  (`voe_editor_models_materials_read`, card 34) and the open material refreshed; after each read in
  `assets_manage.c`, the refresh too. Give the session's call the models if it has none.
- `editor/src/src.md`: entries for the new files and the changed ones (each at most 300 characters).

## Done when
`voe_editor` builds in the folder's check with no warning. The human: import a `.glb` with two
materials; two `.material` files and a `.parts` appear beside it; placed, it shows in plain colours.
