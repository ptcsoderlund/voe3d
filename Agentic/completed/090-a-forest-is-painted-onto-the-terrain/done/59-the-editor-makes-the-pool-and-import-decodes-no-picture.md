# 59 — The editor makes the pool, and Import decodes no picture
folder: editor
after: 51, 52, 55, 56
decisions: 0168, 0429, 0447, 0448

## Change
0448 point 2 in the editor, and the import's own waste: Import read every picture of the
`.glb` only to name its materials.

- `editor/src/models.c` — `voe_editor_models_new` makes a pool of `voe_platform_cores() - 1`
  threads (`base/workers.h`) and sets it on the store (`voe_3d_models_workers_set`);
  `voe_editor_models_destroy` destroys it after the store. Header: the store carries the
  program's loading pool (0448 point 2).
- `editor/src/loading.h`, `editor/src/loading.c` — `voe_editor_loading_prepare` gains
  `voe_base_workers *workers`, passed to `voe_game_models_prepare` (card 56's new parameter), so
  Make LODs and an import's LODs are made over the pool. The prepare work's comment says so.
- `editor/src/main.c` — the prepare's setup gives it
  `voe_3d_models_workers(voe_editor_models_store(models))`.
- `editor/src/model_import.c` — `model_read` for Import reads with
  `voe_assets_model_read_glb_encoded`, no picture decoded; Import textures, which writes the
  pictures out, keeps `voe_assets_model_read_glb`. Header: which read each command uses.
- `editor/src/src.md` — the `models.c`, `loading.c` and `model_import.c` entries.

## Done when
`grep -q "voe_assets_model_read_glb_encoded" editor/src/model_import.c` and
`grep -q "voe_3d_models_workers_set" editor/src/models.c` exit 0, and
`build/debug/editor/voe_editor examples/tank_game --capture "$(mktemp -d)/t.png" 2>&1 | grep -q
"^game: models: "` exits 0 (the start's loaded models print their phases).
