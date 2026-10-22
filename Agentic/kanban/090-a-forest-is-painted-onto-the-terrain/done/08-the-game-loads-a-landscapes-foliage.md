# 08 — The game loads a landscape's foliage
folder: game
after: 06
decisions: 0168, 0426, 0442

## Change
0442 points 1 and 5, the loader's half, shared with the editor through `game/models.h`: a
cooked landscape carries its kinds and copies, and every loaded landscape's kinds are loaded
as model files.

- `game/include/game/landscapes.h` — includes `<assets/landscape.h>`; `voe_game_landscape`
  gains, after `paint`, `const char *const *kinds` (kind_count project-relative `.glb` paths),
  `uint32_t kind_count`, `const voe_assets_landscape_copy *copies` and `uint32_t copy_count`,
  NULL and 0 with none; last, so a table cooked before them still compiles. Header: the example
  shows a kinds array and a copies array; the constraints line names their limits.
- `game/src/models.c` —
  - `voe_game_models_landscapes`: the landscape it builds carries the kinds (asserting each fits
    VOE_ASSETS_LANDSCAPE_KIND_PATH and the count VOE_ASSETS_LANDSCAPE_KINDS) and the copies
    (the store copies them into its own room, `3d/models_foliage.h`).
  - `voe_game_models_update`: after the model rows and emitters, every kind path of every
    loaded landscape entry in the store (`voe_3d_models_count`/`_at`, `entry->landscape`) that
    the store lacks is read and loaded as a row's path is, levels file included, counted in the
    progress total and failures the same way; an empty path skipped.
  - Keep the file under ~600 lines; if it passes, put the kinds' walk in
    `game/src/models_kinds.c` with its header.
- `game/include/game/models.h` (header at its cap) — the comment above `_update` says loaded
  landscapes' kinds are read too; the header's UPDATE paragraph only if a phrase fits the cap.
- `game/tests/models.c` — a table landscape of 4 cells naming one kind, the test's `.glb` as
  the other cases write it into their folder, and two copies: after `voe_game_models_landscapes`
  and `voe_game_models_update` with an empty world, the kind is a loaded entry, the store's grid
  holds both copies as cooked and `voe_3d_models_landscape_placed` gives two; a kind naming a
  missing file is one failure naming that path.
- `game/include/game/game.md`, `game/src/src.md`, `game/tests/tests.md` — entries follow.

## Done when
Test `game/models` passes.
