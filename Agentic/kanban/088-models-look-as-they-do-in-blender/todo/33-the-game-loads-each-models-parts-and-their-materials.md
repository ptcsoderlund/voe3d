# 33 — The game loads each model's parts and their materials
folder: game
after: 23, 32
decisions: 0168, 0236, 0399, 0420

## Change
The loader the game and the editor share takes 0420 point 1's parts table; model rows lost `materials`
in card 31, and this card makes `game` build again. The editor's call breaks; card 34 mends it.

- `game/include/game/materials.h`: `typedef struct { const char *path; voe_assets_parts parts; }
  voe_game_model_parts` (the `.glb`'s project-relative path and its `.parts` values) and `typedef struct
  { const voe_game_model_parts *models; uint32_t count; } voe_game_parts`; `extern const voe_game_parts
  voe_game_parts_cooked`, defined by the cooked `materials.c` beside the materials (card 35). The
  header's example and its COOKED paragraph name both tables.
- `game/include/game/models.h`, `game/src/models_materials.c`: `voe_game_models_materials` gains `const
  voe_game_parts *parts` after `table`. It first gives every loaded model entry its parts
  (`voe_3d_models_parts_set`, NULL for a path the table lacks), then loads every material path a shape
  row, a model entry's part `wears`, or a loaded landscape's layer names that the store lacks, as now.
  `voe_game_models_material_load` reads `emissionmap` into the maps' `emission` as the other three.
  The header's MATERIALS paragraph says so, and that the call is made after a watch too, so a reloaded
  model wears its parts again before it is drawn.
- `game/src/run.c`: hands `&voe_game_parts_cooked` to every `_materials` call.
- `game/tests/models.c`: a parts table naming a table material for the hand-built `.glb`'s material 0:
  after update and materials the entry's part wears it and the material is loaded; a table without the
  `.glb` leaves `wears` empty; a material with an emission map loads it.
- `game/include/game/game.md`, `game/src/src.md`, `game/tests/tests.md`: entries follow (each at most
  300 characters).

## Done when
The test `game/models` passes after the folder's build.
