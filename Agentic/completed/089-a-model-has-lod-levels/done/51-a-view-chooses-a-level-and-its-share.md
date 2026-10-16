# 51 — A view chooses a level and how much of it shows
folder: 3d
after: 50
decisions: 0168, 0425, 0432

## Change
0432 point 3: in a band below a level's bound, that level and the one before share the model's
pixels. The share is worked out with the level, statelessly, so the draws (card 53) ask once.

- `3d/include/3d/model_levels.h`:
  - `typedef struct { uint32_t level; float shown; } voe_3d_model_level_choice;`, commented:
    `level` as `voe_3d_model_level` answers; `shown` in (0, 1], the share of the pixels `level`
    draws, `level − 1` drawing the rest when under 1; 1 for level 0.
  - `voe_3d_model_level_choice voe_3d_model_level_choose(const voe_3d_model_entry *model, const
    voe_scene_transform *placed, voe_math_double3 eye, const voe_render_view *view)`.
  - `voe_3d_model_level` keeps its signature; its comment: `_choose`'s level.
  - Header: THE CHOICE paragraph gains the band: from b_k down to the larger of 0.8 × b_k and
    b_(k+1), `shown` = (b_k − s) ÷ (b_k − low); whole below it, for level 0 and an infinite bound.
- `3d/src/model_levels.c`: `_choose` does what `voe_3d_model_level` does now plus the share; the
  0.8 a named constant with its 0432 reason; `voe_3d_model_level` returns `_choose(...).level`.
- `3d/tests/model_levels.c`: a new case on the hand-filled entry of the choice case (radius 1,
  bounds infinite, 0.2, 0.05) with distances from the projection's P[1][1] as that case computes
  them:
  10. s about 0.19: level 1, `shown` within 0.01 of 0.25;
  11. s about 0.15 (below 0.8 × 0.2): level 1, `shown` 1;
  12. bounds infinite, 0.2, 0.19 and s about 0.195: level 1, `shown` within 0.05 of 0.5;
  13. eye inside the sphere: level 0, `shown` 1;
  14. at each distance of the choice case, `voe_3d_model_level` equals `_choose`'s level.
  Header lists them.
- `3d/tests/tests.md`: the `model_levels.c` entry names the band.

## Done when
The test `3d/model_levels` passes after the folder's build, with cases 10 to 14 in it.
