# 36 — The loader makes a model's levels as its parts file says
folder: game
after: 35
decisions: 0168, 0426, 0430

## Change
0430 point 5: when the loader makes a `.glb`'s levels file it reads `[Levels]` from the `.parts`
beside it and hands them to `voe_3d_model_levels_make`, which now takes them
(`3d/include/3d/model_levels.h`, card 35; the type in `assets/include/assets/parts.h`, card 33).

- `game/src/models.c`: `levels_for` (around line 111), only on the path where it makes, gets the
  model's levels from a new static helper: the path's `.glb` ending (any case, `is_glb` already
  says so) made `.parts`, joined onto the folder as the `.glb` is, read with
  `voe_platform_file_read` on scratch and parsed with `voe_assets_parts_read`; a file that is
  missing gives `VOE_ASSETS_LEVELS_DEFAULT` with nothing on stderr, one that will not parse gives
  it too (the parts reader has said why). The levels passed to `_make`. Header: the sentence on the
  `.glb`'s levels file names the `.parts` read and its default. 385 lines now; if it passes ~450,
  move `levels_for`, its helpers and `voe_game_models_prepare` into a new `game/src/models_levels.c`
  with a header comment, declared in a small internal header.
- `game/include/game/models.h`: the LEVELS paragraph gains a sentence: the file is made with the
  count and strength of the `.parts` beside the `.glb`, the defaults when none (0430 point 5), so a
  shipped game, which has none, uses the files it ships.
- `game/CMakeLists.txt`: `assets` joins the DEPENDS list, since the folder now calls it.
- `game/src/src.md`: the `models.c` entry names the `.parts`'s levels.
- `game/tests/models.c`: a builder beside `glb_build` for a flat grid of 8 × 8 quads in the plane
  z = 0: 81 positions and 384 32-bit indices (componentType 5125, an indices accessor and its own
  buffer view), JSON and BIN laid out as `glb_build` does. A new case at the end of THE LEVELS, on its
  store, with the grid written as `grid.glb` in the scratch folder:
  1. prepare of `grid.glb` with no `grid.parts`: true, made, and the file read
     (`voe_3d_model_levels_read`) has `level_count` 3 or more;
  2. `grid.parts` written as `[Parts]`, `[Levels]`, `count=2`; prepare with remake: the file read
     has `level_count` 2;
  3. `grid.parts` written as `[Parts]`, `[Levels]`, `count=9`; prepare with remake: true, and
     `level_count` 3 or more again (the default).
  The header's LEVELS paragraph names the case. `game/tests/tests.md`: the `models.c` entry names
  the parts file's count.

## Done when
The test `game/models` passes after the folder's build, with the new case in it.
