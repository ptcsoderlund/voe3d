# 35 — A model's levels are made as many and as hard as asked
folder: 3d
after: 33, 34
decisions: 0168, 0425, 0426, 0430

## Change
0430 points 2, 3 and 7: the levels file is made with a model's count and strength
(`voe_assets_levels`, `assets/include/assets/parts.h`, card 33), using the angle card 34 gave
`voe_3d_simplify` (`3d/src/simplify.h`).

- `3d/include/3d/model_levels.h`: includes `<assets/parts.h>`. `voe_3d_model_levels_make` gains
  `voe_assets_levels levels` after `glb_size`; its comment says it makes at most `levels.count`
  levels in all. Header: THE LEVELS paragraph says the count and the three strengths as 0430 point 2
  gives them (angle, cut per level, limit as a share of the radius), the three-quarters rule kept;
  the usage example passes `VOE_ASSETS_LEVELS_DEFAULT`; the layout's version is 2, and a sentence
  that version 1 files are stale (0430 point 7) and that the file does not record the count or
  strength.
- `3d/src/model_levels.c`: `VERSION` 2. One static table by strength of the angle (0, 60, 180), the
  shift per level (1, 1, 2) and the limit's divisor of the radius (4, 4, 2); a level k's target per
  part its triangles shifted by k × the shift; the loop stops at `levels.count`; the angle passed to
  `voe_3d_simplify`. A `static_assert` that `VOE_ASSETS_LEVELS` equals `VOE_3D_MODEL_LEVELS`; an
  assert on the count's range. Header: A PART'S TARGET paragraph names the strength's shift.
- `3d/tests/model_levels.c`: existing calls pass `VOE_ASSETS_LEVELS_DEFAULT`. New cases:
  6. the 64 × 32 sphere with count 2: read gives `level_count` 2 at most; with count 1: exactly 1;
  7. the sphere at STRONG: level 1 has fewer triangles than level 1 at MEDIUM;
  8. a file of version 1 (the default file with its version field set to 1): read false.
  The header lists the eight.
- `3d/tests/draw_levels.c`, `3d/tests/models_levels.c`: their calls of `_make` pass
  `VOE_ASSETS_LEVELS_DEFAULT`. Nothing else changes in them.
- `3d/tests/tests.md`: the `model_levels.c` entry names the count and strength.
- `3d/3d.md`: the `include/3d/model_levels.h` entry says how many and how hard, in a phrase.

The one caller outside the folder, `game/src/models.c`, is card 36's; leave it.

## Done when
The tests `3d/model_levels`, `3d/draw_levels` and `3d/models_levels` pass after the folder's build,
with cases 6 to 8 in the first.
