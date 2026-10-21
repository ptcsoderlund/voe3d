# 53 — A model's LODs are made over the pool, without its pictures
folder: 3d
after: 48, 50, 52
decisions: 0168, 0447, 0448

## Change
0448 point 3, making LODs: the first import of a big model spends its time here. Each part
of each level is simplified from level 0 on its own (`3d/src/simplify.h`), so every pair is a
task of card 50's pool; and nothing here needs a picture decoded.

- `3d/include/3d/model_levels.h` — `voe_3d_model_levels_make` takes `voe_base_workers *workers`
  after `arena` (NULL runs in order on the caller). Its comment: the `.glb` is parsed with its
  pictures left encoded; each (level, part) simplified as a task with an arena of its own; the
  bytes made are the same whatever the pool. `voe_3d_model_levels_triangles`' comment: no
  picture decoded.
- `3d/src/model_levels.c` —
  - every `voe_assets_model_read_glb` here becomes card 52's
    `voe_assets_model_read_glb_encoded`.
  - the make of made levels: level 0 baked as now; an index buffer of the part's level-0 index
    count pushed on `arena` for every level and part before the run; one task per (level,
    part) runs `voe_3d_simplify` into its buffer on an arena of its own (`voe_base_arena_new`,
    destroyed in the task), keeping its count and error; after the run the levels are judged in
    order exactly as now, the first level not below the one before ending the list, its
    buffers and the later ones' rewound off `arena`. Own levels stay as they are.
  - Header: A LEVEL IS MADE WHOLE paragraph says all levels are simplified at once over the pool
    before any is judged, and what that holds in memory (a buffer per level and part).
- `3d/tests/model_levels.c`, `3d/tests/models_levels.c`, `3d/tests/draw_levels.c` — every
  `_make` call passes NULL. A new check in `model_levels.c`: the sphere's file made with a
  pool of 3 (`voe_base_workers_new(3)`) is byte for byte the file made with NULL, at the
  default count and at six levels.
- `3d/src/src.md`, `3d/tests/tests.md` — the `model_levels.c` entries.

`game/src/models.c`'s call is card 56's; `game` does not build between the two cards.

## Done when
`ctest --test-dir build/debug -R '^3d/(model_levels|models_levels|draw_levels)$'` passes with
the new check.
