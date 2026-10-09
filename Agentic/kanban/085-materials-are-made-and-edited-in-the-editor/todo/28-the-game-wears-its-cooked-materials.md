# 28 — The game wears its cooked materials
folder: game
after: 24, 08, 27
decisions: 0168, 0399

## Change
0399 point 7: a shipped game and Play load the cooked table.

- `game/src/run.c` — `#include <game/materials.h>`; right after each `voe_game_models_update` call
  (at the start, after the cooked landscapes, and the two in the run), `voe_game_models_materials`
  (card 24) with `&voe_game_materials_cooked`, the same world, store, device, folder and scratch,
  its result treated as that update's is. The header's list of what `run.c` alone names gains
  `voe_game_materials_cooked`.
- `game/include/game/run.h` — where it names what a game tree's cooked files define, add
  `materials.c` defining `voe_game_materials_cooked`.

Any program or test in the tree that defines `voe_game_landscapes_cooked` for itself defines an
empty `voe_game_materials_cooked` beside it. Update `game/src/src.md`'s `run.c` line.

## Done when
`grep -c 'voe_game_models_materials' game/src/run.c` prints 3, and the game folder builds and its
tests pass. Human: Play on a project whose cube wears a material shows it as the editor does.
