# 77 — The game loads each kind's base size
folder: game
after: 75, 76
decisions: 0168, 0442, 0459, 0460

## Change
0460 point 4: a cooked landscape carries its kinds' base sizes, so Play and Ship stand each
copy at the size the editor shows.

- `game/include/game/landscapes.h` — `voe_game_landscape` gains, last, `const uint16_t
  *kind_sizes`: kind_count sizes in thousandths as `.landscape`'s `size<k>=`, 0 meaning 1×;
  NULL every kind 1×. Last, so a table cooked before it still compiles. Header: the example's
  entry gains a sizes array; the constraints line names its range and length.
- `game/src/models.c` — where `voe_game_models_landscapes` copies the cooked kinds into the
  landscape it builds, it copies the sizes too when not NULL (each asserted 0 or within
  VOE_ASSETS_LANDSCAPE_KIND_SIZE_MIN–_MAX); NULL leaves them 0.
- `game/tests/models.c` — the foliage case's table landscape gains `kind_sizes` {2000}: after
  the load, `voe_3d_models_landscape_placed` gives each copy a scale of twice its own; a
  landscape with `kind_sizes` NULL keeps its own scale.
- `game/include/game/game.md`, `game/src/src.md`, `game/tests/tests.md` — the landscapes and
  models entries name the base sizes.

## Done when
Test `game/models` passes.

## Blocked
The card is `after: 75, 76`, and neither is in this tree: both are still in `todo/`, so `voe_assets_landscape` has no `kind_sizes`, `VOE_ASSETS_LANDSCAPE_KIND_SIZE_MIN`/`_MAX` do not exist, and the store does not multiply a copy's scale by its kind's base size. Without them the copy in `game/src/models.c` cannot compile and the test's twice-its-scale check cannot pass. To unblock it, run this card again once cards 75 and 76 are committed on the feature branch.
