# 07 — A game loads its cooked landscapes
folder: game
after: 03
decisions: 0168, 0379

## Change
A game's landscapes come from its cooked table, never a file, and nothing re-reads them by stamp (0379
points 2 and 7).

- New `game/include/game/landscapes.h` — `voe_game_landscape`: `const char *path` (project-relative, as
  a model row names it), `float size`, `uint32_t cells`, `const int32_t *millimetres` ((cells + 1)²);
  `voe_game_landscapes`: `const voe_game_landscape *landscapes; uint32_t count`;
  `extern const voe_game_landscapes voe_game_landscapes_cooked;`. Header shaped as `game/prefabs.h`'s:
  defined by the game tree's cooked `landscapes.c`, only run.c names it, why cooked (0236).
- `game/include/game/models.h` / `game/src/models.c` — new
  `voe_game_models_failures voe_game_models_landscapes(voe_3d_models *, voe_render_device *,
  const voe_game_landscapes *, voe_base_arena *scratch)`: each turned into metres in scratch and handed
  to `voe_3d_models_load_landscape` at stamp 0, failures counted as the update counts them. The watch
  skips every entry whose `landscape` is set. Header points for both.
- `game/src/run.c` — the start's work loads `voe_game_landscapes_cooked` this way before its first
  `voe_game_models_update`, so a scene's landscape rows find their entries held.
- `game/tests/models.c` gains `landscapes_load_from_a_table` and `watch_leaves_a_landscape_alone`.
- `game/include/game/game.md` gains `landscapes.h` and its `models.h` entry names the table;
  `game/src/src.md`'s `run.c` and `models.c` entries and `game/tests/tests.md` updated.

## Done when
`ctest --test-dir build/debug -R '^game/models'` passes with the two tests above.
