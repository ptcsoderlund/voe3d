# 03 — A cooked landscape carries its kinds' looks and its growth
folder: game
after: 02
decisions: 0168, 0473, 0476

## Change
0476 point 4's game side: the cooked table carries what card 01 added to the grid, so the store
grows the same copies at load in Play and a shipped game. The editor's cook follows in card 09.

- `game/include/game/landscapes.h` — `voe_game_landscape`: `kind_sizes` becomes
  `const voe_assets_landscape_look *looks` (kind_count looks, NULL every kind the default look);
  a new last field `const uint16_t *grows` (layer_count × kind_count densities in tenths, layer
  by layer, NULL none grown). The header's sketch, its first paragraph and its Constraints
  follow (each look's ranges as assets/landscape.h's; each density 0 to
  VOE_ASSETS_LANDSCAPE_GROW_MAX).
- `game/src/models.c` — where a cooked landscape's grid is filled (the kinds' base sizes today):
  each kind's look copied, asserted in range, or `voe_assets_landscape_look_default()` when
  `looks` is NULL; `grows` copied into the grid's `grows[l][k]`, asserted in range, all 0 when
  NULL.
- `game/tests/models.c` — any cooked table naming `kind_sizes` names `looks`; a new case: a
  cooked landscape with two layers, one kind, a look with spreads and shadowless set, and layer
  1 growing the kind, loads with that look and those grows on its store entry's grid.
- `game/src/src.md`, `game/include/game/game.md` — the `models.c` and `landscapes.h` entries
  say looks and growth.

## Done when
The `game/models` test passes with the new case.
