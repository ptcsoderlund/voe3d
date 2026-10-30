# 02 — Bounds are capped and spread when read
folder: 3d
after: none
decisions: 0168, 0495, 0500

## Change
Every LOD bound is made in the store's `3d/src/models_levels.c` (`error_bound`, then each made no
larger than the one before). The rule moves into one public pure function the store and the LODs
panel (card 09) both call, and gains 0500 point 5's cap and 0495's 1.25 spread.

- `3d/include/3d/model_levels.h`:
  - Add `void voe_3d_model_levels_bounds(float radius, const float errors[VOE_3D_MODEL_LEVELS],
    const float swaps[VOE_3D_MODEL_LEVELS], uint32_t level_count, bool measured,
    float bounds[VOE_3D_MODEL_LEVELS])`: `bounds[0]` INFINITY; level k ≥ 1 is
    0.002 × radius ÷ errors[k] ÷ swaps[k] when `measured` (INFINITY for error 0), 2^-k when not
    (an own file with no measures, as now); then each capped at 1; then each at most the one
    before ÷ 1.25. Entries past `level_count` unwritten.
  - Add `float voe_3d_model_level_metres(float radius, float bound, float fov_y)`: the distance at
    which a model of that radius at scale 1 is that size on screen, radius ÷ tan(fov_y ÷ 2) ÷ bound;
    0 for an infinite bound.
  - Header comment, THE LEVELS paragraph: the "pushed out to it / the earlier never shows /
    swaps 1 1 1 2 1 skip level 4" lines become: capped at s = 1, each at least 1.25 times as far
    as the one before, applied when read and never written back (0500 points 2, 4, 5). The choice
    paragraph stays for now (card 05 rewrites it).
- `3d/src/model_levels.c`: define both, with the 0.002 and 1.25 as named constants with their
  decisions; the store's `SCREEN_ERROR` moves here.
- `3d/src/models_levels.c`: `upload_own` and `upload_made` fill each entry's `levels[k].bound`
  through `voe_3d_model_levels_bounds` (measured as `prepared->measured` now decides; a made file
  is measured); `error_bound` and the hand-made non-increasing step go. Its header comment says
  the bounds come from that one function.
- `3d/include/3d/models.h`: the comment on `voe_3d_model_level_parts.bound` says capped and spread.
- `3d/tests/model_levels.c`: add `bounds_are_capped_and_spread` — swaps ×0.2, ×0.5, ×0.5, ×1, ×0.8
  over errors that rise per level give bounds each ≤ the one before ÷ 1.25 and none above 1; an
  error of 0 gives 1 (capped), not infinity; unmeasured gives 2^-k spread; and
  `level_metres_follow_the_lens` — at fov 60° a bound of 1 is radius × √3 metres.
- `3d/tests/models_levels.c`: any check whose expected bound or level changes with the cap or the
  spread is updated to the new rule, each with a comment naming 0500; none removed. Any other 3d
  test whose expected level moves with the new bounds (e.g. `3d/tests/draw_levels.c`) likewise.
- `3d/include/3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md`: entries changed as above.

## Meaning
- Holds: each LOD swaps in at least 1.25× as far as the one before; bounds capped at s = 1 before
  spreading; own LODs on the 2^-k fallback spread like any other (0495, 0500).
- Holds: spreading happens when read — load, cook, "LODs of <file>" — never written back (0500 pt 4).
- Believed (observed): every bound is made only in `models_levels.c`; the cooked file carries
  errors and swaps, not bounds, so this change reaches load, cook and Play alike.

## Done when
The tests `3d/model_levels` and `3d/models_levels` pass, and
`! grep -q error_bound 3d/src/models_levels.c` exits 0.
