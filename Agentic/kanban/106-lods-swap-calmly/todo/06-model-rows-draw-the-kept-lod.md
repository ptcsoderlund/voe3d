# 06 — Model rows draw the kept LOD
folder: 3d
after: 05
decisions: 0168, 0432, 0500, 0501

## Change
Model rows choose through the view's memory (card 05, `3d/include/3d/lods_kept.h`; read its
header) instead of the stateless band. The dither record (`object.dither`, 0432 point 5) is used as
now: d in (0,1] keeps the pixels under d, d in [−1,0) the rest; render and shaders do not change.

- `3d/include/3d/draw_system.h`: `voe_3d_frame` gains `voe_3d_lods_kept *lods` beside `casters`:
  this target's memory, NULL for none; `_frame` leaves it NULL, the caller begins it and sets it
  each frame; only the draws write it. The model draw's and the shadows call's comments say they
  choose through it. Include `3d/lods_kept.h`.
- `3d/src/draw_models.c`: the row's choice (line ~209) is
  `voe_3d_lods_kept_choose(frame->lods, voe_3d_lods_kept_row_key(entity), ...)`, the entity the
  row's from `voe_3d_model_entities`. A dissolving row draws `level` with `shown` and `from` with
  −`shown` (today's `level − 1` becomes `from`, which may be finer or coarser); a settled row draws
  `level` whole; a fading row's held parts draw `level` whole. The assert at ~221 follows the new
  struct. Its header comment: the band paragraph becomes the 0.25 s dissolve by the frame's
  memory, and "holds nothing between frames" says the memory is the caller's.
- `3d/src/draw_models.h`: comment on `voe_3d_draw_models` says the level comes from `frame->lods`.
- `3d/src/draw_shadows.c`: the level at line ~218 (not the bounce's `coarsest`) is the same
  choose's `level`, drawn whole; a NULL memory gives the stateless level as today.
- `3d/tests/draw_levels.c`: `a_band_dissolves` becomes `a_swap_dissolves` — with a memory, a sphere
  first seen near draws level 0 alone; moved past level 1's bound one frame later it draws
  level 1 and level 0; 0.3 s later level 1 alone; moved back just inside the bound it still draws
  level 1 alone; 10% nearer it draws both again. Keep the fading and no-levels checks. The file's
  top comment and `3d/tests/tests.md` entry follow.
- `3d/src/src.md`: `draw_models` entry mentions the memory.

## Meaning
- Holds: every swap dissolves old and new over 0.25 s, replacing 0432's band; a settled copy is
  drawn whole; shadows and held parts draw the one LOD the copy keeps; bounce draws the last LOD
  (0500 pt 1, 0495 pt 2).
- Gap answered: the shader's dither comes from the object record's `dither`, which carries a
  fraction and its complement already; no render change.

## Done when
The tests `3d/draw_levels` and `3d/draw_system` pass.
