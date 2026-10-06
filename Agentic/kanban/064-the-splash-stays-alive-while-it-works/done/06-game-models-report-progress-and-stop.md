# 06 — Reading models reports progress and stops when asked
folder: game
after: 05
decisions: 0168, 0362, 0370

## Change
- `game/include/game/models.h`, `game/src/models.c`: `voe_game_models_update` gains a last
  parameter `voe_game_progress *progress` (`game/progress.h`), NULL outside a splash wait. With
  one: the paths this call will read are counted first, then "Loading models" done/total is set
  before each file; once stop is asked it returns before the next file, the failures so far
  counted; paths left unread are read by a later call as now. The header says so.
- Call sites: the three in `game/src/run.c` pass NULL (card 08 changes them); every one in
  `game/tests/models.c` passes NULL. `editor/src/models.c` breaks until card 07; do not touch it.
- `game/tests/models.c`: an update with a progress over the hand-built `.glb` ends with done equal
  to total and total 1; one with stop already set reads nothing and a second update without it
  loads the file.
- `game/include/game/game.md`, `game/tests/tests.md`: the entries mention progress.

## Done when
`ctest --test-dir build/debug -R '^game/models$'` passes.
