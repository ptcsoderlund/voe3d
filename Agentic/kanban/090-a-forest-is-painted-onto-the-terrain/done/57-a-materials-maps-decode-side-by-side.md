# 57 — A material's maps decode side by side
folder: game
after: 56
decisions: 0168, 0445, 0448

## Change
0448 points 3 and 4 for materials: the lumberhill start reads ten 4k maps for its materials one
after another; they decode on the store's pool, and the upload stays on the calling thread.

- `game/src/models_materials.c` —
  - `voe_game_models_materials`: after the parts are given, every path it would load
    (shapes' materials, parts' `wears`, landscapes' layers, the store lacking it, the table
    having it) gathered first; a path the table lacks kept failed and counted as now. The
    gathered ones in chunks of `voe_base_workers_threads(voe_3d_models_workers(models))`
    materials: reading (one task a material, its maps read into an arena of its own under its
    path as the subject), decoding pictures (one task a map, `voe_3d_model_picture_decode` into
    an arena of its own under the material's path and "the <kind> map `path`", as now), then
    uploading in order through `voe_3d_models_put_material`, a failure kept and counted as now,
    a full store ending the call; each material's arenas destroyed after its put.
  - `voe_game_models_material_load` the same for its one material, its four maps decoded side
    by side.
  - the line through card 56's times record: `game: materials: <n> files in …: reading …,
    decoding pictures …, uploading …`, once per call that read a material.
  - Header: the phases and the line; "rewound after each material" becomes each material's
    own arenas.
- `game/src/models_batch.h`, `game/src/models_batch.c` — the times line takes the noun and
  which phases it prints, if card 56 left it for models alone.
- `game/include/game/models.h` — MATERIALS paragraph: maps decoded side by side on the store's
  pool, uploaded in order, the line.
- `game/src/src.md` — the `models_materials.c` entry.
- `game/tests/models.c` — new check: the test's materials (a table material with maps) loaded
  into a store with a pool of 3 set give the same entries and failures as without; a table
  material with a broken map is kept failed with its map's path in the kept error, with the
  pool as without.
- `game/tests/tests.md` — the `models.c` entry names the check.

## Done when
`ctest --test-dir build/debug -R '^game/(models|models_levels|frame)$'` passes with the new check.
