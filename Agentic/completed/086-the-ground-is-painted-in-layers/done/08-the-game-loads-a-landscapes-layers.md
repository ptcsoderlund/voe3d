# 08 — The game loads a landscape's layers
folder: game
after: 05
decisions: 0168, 0399, 0402

## Change
0402 point 8, the game's half, shared with the editor through `game/models.h`.

- `game/include/game/landscapes.h` — `voe_game_landscape` gains, after `millimetres`, `const
  char *const *layers` (layer_count project-relative `.material` paths), `uint32_t layer_count`
  and `const uint8_t *paint`: one RGBA8 plane of (cells + 1)² texels up to four layers, two past
  four, as `authoring/landscape_cook.h` cooks it; NULL with no layers. Added at the end so a
  cooked table written before them still compiles (missing fields read nought). Header: the
  example and the constraints line extended.
- `game/src/models.c` — `voe_game_models_landscapes`: the landscape it builds carries the layer
  paths (asserting each fits `VOE_ASSETS_MATERIAL_PATH`) and, when `paint` is not NULL, a paint
  of both planes in scratch, the second all nought when only one was cooked; else paint NULL.
- `game/src/models_materials.c` — `voe_game_models_materials` also walks the store's loaded
  landscape entries (`entry->landscape`) and loads every layer path the store lacks from the
  table, failing one the table has not, counted as a row's is. `game/include/game/models.h`'s
  MATERIALS paragraph and the function's comment say landscapes' layers are loaded too.
- `game/src/run.c` — read its header only: if the landscapes are loaded after the first
  materials pass, nothing changes, since each models update is followed by one; else say so in
  your report.
- `game/tests/models.c` — a table landscape of 4 cells with two layers naming two table materials
  and a painted point: after `voe_game_models_landscapes` and `voe_game_models_materials` with
  an empty world, both materials are loaded entries, the store's grid holds both layer paths,
  and the painted point's bytes are as cooked; a layer naming no table material is one failure.
- `game/include/game/game.md`, `game/src/src.md`, `game/tests/tests.md` — entries follow.

## Done when
Test `game/models` passes.
