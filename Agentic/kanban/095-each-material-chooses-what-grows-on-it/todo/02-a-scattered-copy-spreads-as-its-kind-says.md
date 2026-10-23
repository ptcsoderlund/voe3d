# 02 — A scattered copy spreads as its kind says
folder: 3d
after: 01
decisions: 0168, 0442, 0469, 0476

## Change
0476 point 1 in the arithmetic: the brush keeps Density and the picked kinds; a copy's size,
turn and tilt spreads are its kind's look (`voe_assets_landscape_look`, card 01). Card 01 left
this folder not building (`kind_sizes` is now `looks`); this card makes it build again. The
editor, which fills the brush, follows in card 07.

- `3d/include/3d/foliage.h` — `voe_3d_foliage_brush` loses `size_vary`, `turn_vary` and `lean`.
  The header's A SCATTER STAMP paragraph: scale 1 ± the kept kind's `size_vary` %, turn ± its
  `turn_vary`°, lean its `tilt` % of 255 (0476 point 1); the sketch follows.
- `3d/src/foliage.c` — the stamp draws the kind first, then its scale, turn and lean from that
  kind's look; the draws stay from the caller's xorshift.
- `3d/src/models_foliage.c`, `3d/include/3d/models_foliage.h` — `kind_sizes[i]` uses become
  `looks[i].size`; a kind added by `_kind_set` takes `voe_assets_landscape_look_default()` and
  every layer's grow of it 0; a replaced kind keeps its look. The `_kind_set` comment says so.
- `3d/tests/foliage.c`, `3d/tests/bounce_foliage.c`, `3d/tests/models_landscape.c` — wherever a
  brush sets the three spreads, the test sets the kinds' looks instead (the same numbers, in %,
  degrees and %); `kind_sizes` uses become `looks[k].size`. New case in `foliage.c`: a kind with
  size_vary 0, turn_vary 0 and tilt 100 scatters copies all at scale 1000, turn 0 and lean 255,
  and a second kind picked with it at the defaults scatters copies whose scales differ.
- `3d/src/src.md` — the `foliage.c` entry says the spreads are the kind's.

## Done when
The `3d/foliage`, `3d/bounce_foliage` and `3d/models_landscape` tests pass.
