# 42 — The game grows no species
folder: game
after: 41, 38
decisions: 0168, 0520

## Change
Take out of `game/` what card 06 `9af6ef1c` and card 18 `4ee17329` added, seen with
`git show <hash> -- game`. Later cards' changes to the same files stay.

- Delete `game/include/game/trees.h`, `game/src/trees.c`, `game/tests/trees.c`.
- `game/include/game/models.h`, `game/src/models.c`: the loader no longer skips `.tree` paths; every kind
  path is a glb it loads.
- `game/src/run.c`: Play and Ship grow no forest at start.
- Entries in `game/include/game/game.md`, `game/src/src.md`, `game/tests/tests.md`.

## Done when
`cmake --build --preset debug --target voe_game` exits 0, and
`git grep -nE 'game/trees\.h|tree_grow|species' -- game` prints nothing.
