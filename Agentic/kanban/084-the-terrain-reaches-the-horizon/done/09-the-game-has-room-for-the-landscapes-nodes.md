# 09 — The game has room for the landscape's nodes
folder: game
after: 08
decisions: 0168, 0396

## Change
Cards 06 to 08 changed `3d/models.h`: a landscape is one grid part, drawn as up to
`VOE_3D_LANDSCAPE_NODES` objects in each pass, for up to `VOE_3D_LANDSCAPES_DRAWN` rows.

- `game/include/game/frame.h`, `VOE_GAME_CAPACITIES` and its comment: `objects` adds
  `VOE_3D_LANDSCAPES_DRAWN × VOE_3D_LANDSCAPE_NODES` times the same pass count the world's
  drawn objects are multiplied by (the window pass, every cascade of every light, the point
  shadows, the captures, the bounce sun maps). `heights_texels` stays nought: the game never
  sculpts, and its landscapes are uploaded whole at load.
- `game/include/game/landscapes.h`: `cells` is a multiple of 4 from 4 to
  `VOE_ASSETS_LANDSCAPE_CELLS_MAX` (0396 point 1), not 512.
- `game/tests/models.c`: its capacities and their comment hold the landscape as the store now
  makes it (the grid geometry, its ground and its twin, the heights texture), not 16 chunks; the
  landscape case also checks the entry has one part.
- `game/tests/frame.c`: a case beside the lamp case: a 64-cell, 512 m landscape table loaded with
  `voe_game_models_landscapes` and a row wearing it under the sun; two frames with
  `VOE_GAME_CAPACITIES` both come back true. Its header names the case.
- `game/tests/tests.md` follows.

## Done when
`game/tests/models.c` and `game/tests/frame.c` pass.
