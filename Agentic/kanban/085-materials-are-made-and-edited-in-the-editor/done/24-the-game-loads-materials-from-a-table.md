# 24 — The game loads materials from a table
folder: game
after: 23
decisions: 0168, 0399

## Change
0399 point 7: one loader for the game and the editor, fed by a table of path and values.

- New `game/include/game/materials.h` — `voe_game_material { const char *path; voe_assets_material_file
  values; }`, `voe_game_materials { const voe_game_material *materials; uint32_t count; }` and
  `extern const voe_game_materials voe_game_materials_cooked;`. It includes `<3d/models.h>` for the
  values' type and names no `assets` header (game has no `assets` edge; check step 5). Header points
  as `game/include/game/landscapes.h`'s: defined by the cooked `materials.c`, only `run.c` names it,
  cooked because a game parses no project text (0236); the editor builds its own from files.
- `game/include/game/models.h` and new `game/src/models_materials.c`:
  - `[[nodiscard]] bool voe_game_models_material_load(voe_3d_models *models, voe_render_device
    *device, const char *folder, const voe_game_material *material, uint64_t stamp, voe_base_arena
    *scratch, voe_base_error *error)` — each non-empty map path joined onto `folder`, read into scratch
    as `models.c` reads a file, and handed to `voe_3d_models_load_material` (card 21); a map that
    will not read fails with the error a model file that will not read gives, the entry kept failed
    through `voe_3d_models_fail`. Scratch rewound.
  - `voe_game_models_failures voe_game_models_materials(const voe_ecs_world *world, voe_3d_models
    *models, voe_render_device *device, const char *folder, const voe_game_materials *table,
    voe_base_arena *scratch)` — every non-empty shape `material` and model `materials[i]` path the
    store holds no entry for: found in `table`, loaded at stamp 0 by the call above; not in it,
    `voe_3d_models_fail`ed. Each failure counted and one stderr line, as an update's; so a broken one
    is tried once. Between frames.
  - `voe_game_models_watch` (in `game/src/models.c`) skips an entry that is `material`, as it skips
    a landscape: its path is no model file, and the editor reloads a changed one itself.
  Update `models.h`'s header with a MATERIALS paragraph and the watch's exception.
- `game/include/game/game.md` and `game/src/src.md` — the new files.

Test: add a MATERIALS case to `game/tests/models.c` (its header says how files and PNG bytes are
made): a world with a shape naming `Assets/m.material`, a table holding it with `colour_map` naming
a 1×1 PNG written into the scratch folder; after the call the store finds it loaded and `material`;
a second shape naming a path not in the table is one failure, named, and a second call counts none; a watch with
`m.material` present on disk as text leaves the entry loaded and reports nothing.
Update the test's header.

## Done when
`ctest --test-dir build/debug -R '^game/models$'` passes with the new case.
