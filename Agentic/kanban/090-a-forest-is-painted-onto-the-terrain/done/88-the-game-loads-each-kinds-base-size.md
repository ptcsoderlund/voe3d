# 88 — The game loads each kind's base size
folder: game
after: 75, 87
decisions: 0168, 0442, 0459, 0460

## Change
0460 point 4: a cooked landscape carries its kinds' base sizes, so Play and Ship stand each
copy at the size the editor shows. Card 75 gave `voe_assets_landscape` its `kind_sizes` and
`VOE_ASSETS_LANDSCAPE_KIND_SIZE_MIN`/`_MAX`; card 87 made the store place a copy at its kind's
size times its own. This card only carries the sizes from the cooked table into the grid.

- `game/include/game/landscapes.h` — `voe_game_landscape` gains, last, `const uint16_t
  *kind_sizes`: kind_count sizes in thousandths as `.landscape`'s `size<k>=`, 0 meaning 1×;
  NULL every kind 1×. Last, so a table cooked before it still compiles. Header: the example's
  entry gains a sizes array; the constraints line names its range and length.
- `game/src/models.c` — where `voe_game_models_landscapes` copies the cooked kinds into the
  landscape it builds, it copies the sizes into `kind_sizes` too when not NULL (each asserted
  0 or within VOE_ASSETS_LANDSCAPE_KIND_SIZE_MIN–_MAX); NULL leaves them 0.
- `game/tests/models.c` — the foliage case's table landscape gains `kind_sizes` {2000}: after
  the load, `voe_3d_models_landscape_placed` gives each copy a scale of twice its own; a
  landscape with `kind_sizes` NULL keeps its own scale.
- `game/include/game/game.md`, `game/src/src.md`, `game/tests/tests.md` — the landscapes and
  models entries name the base sizes.

## Done when
Test `game/models` passes.
