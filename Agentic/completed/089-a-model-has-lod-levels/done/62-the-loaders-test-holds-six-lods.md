# 62 — The loader's test has room for six LODs
folder: game
after: 61
decisions: 0168, 0433

## Change
0433: a model now makes up to six levels by default (`VOE_3D_MODEL_LEVELS` 6,
`3d/include/3d/models.h`, card 61), so the grid of the loader's levels case can make more than
its device room holds.

- `game/tests/models.c`: `CAPACITIES.geometries` (around line 122) becomes
  `VOE_3D_MODEL_LEVELS + 2`; the comment above it says the grid's levels up to
  VOE_3D_MODEL_LEVELS, not four. If the test then runs out of vertices or indices, raise those
  rooms by the same reasoning, said in that comment. The cases stay: `count=2` still gives 2,
  `count=9` still falls back to the default.

Nothing in `game/src` changes: the loader already takes `VOE_ASSETS_LEVELS_DEFAULT`.

## Done when
The test `game/models` passes after the folder's build.
