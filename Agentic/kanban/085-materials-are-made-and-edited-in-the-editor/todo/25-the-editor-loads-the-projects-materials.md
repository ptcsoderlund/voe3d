# 25 — The editor loads the project's materials
folder: editor
after: 24, 10
decisions: 0168, 0399

## Change
0399 point 7, the editor's side: the project's `.material` files as `game/materials.h`'s table, loaded
into the store for every row that names one.

- New `editor/src/materials.h` / `materials.c`:
  - `VOE_EDITOR_MATERIALS` 256; `voe_editor_materials` holding that many `voe_game_material` rows
    with their paths' bytes, and a count.
  - `void voe_editor_materials_read(voe_editor_materials *materials, const char *folder,
    voe_base_arena *scratch)` — every `.material` under `Assets/` (`editor/src/game_tree_find.h`'s
    walk), read with `platform` and parsed by `assets/material.h`; one that will not read or parse is
    left out with a stderr line; past 256 the rest are left out, said once; NULL folder empties.
  - `voe_game_materials voe_editor_materials_table(const voe_editor_materials *materials)`.
  - `voe_game_material *voe_editor_materials_find(voe_editor_materials *materials, const char
    *path)` — NULL when none.
  Header points: the table is what the cook writes (card 27), read again on a project and after each
  Assets command (card 26), edited in place by the Inspector.
- `editor/src/models.h` / `models.c` — `voe_editor_models` holds one, malloced with it. On a
  different folder it is read; every `voe_editor_models_update` calls `voe_game_models_materials`
  (card 24) with it after the models; `voe_editor_models_frame` calls
  `voe_3d_models_material_frame`. New `voe_editor_materials *voe_editor_models_materials(
  voe_editor_models *models)` and `void voe_editor_models_materials_read(voe_editor_models *models,
  const char *folder, voe_base_arena *scratch)` for card 26. Update the header's points.

Add `materials.h` / `.c` to `editor/src/src.md` and change its `models.h` line.

## Done when
`grep -n 'voe_game_models_materials' editor/src/models.c` and `grep -n
'voe_3d_models_material_frame' editor/src/models.c` each find a call, and the editor builds.
