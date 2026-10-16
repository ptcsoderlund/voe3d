# 53 — A model dissolves from one level into the next
folder: 3d
after: 49, 51, 52
decisions: 0168, 0425, 0432

## Change
0432 points 3 and 4: in a level's band the camera draws the model twice, the chosen level on
`shown` of its pixels and the one before on the rest, through `render`'s object `dither`
(`render/include/render/device.h`, card 49) and `voe_3d_model_level_choose`
(`3d/include/3d/model_levels.h`, card 51).

- `3d/src/draw_models.c` (card 52), the model-row walk: the row's level from
  `voe_3d_model_level_choose` in place of `voe_3d_model_level`. A part drawn at once (world layer,
  solid or cut-out, row not fading) with `shown` under 1: drawn at the chosen level with the
  object's `dither` = `shown`, then the same part worn at `level − 1` with `dither` = −`shown`,
  each skipped where its geometry has generation 0. A held part (blended, fading row, overlay
  layer) is drawn whole at the chosen level, as now, so the held groups' sizes do not change. Its
  header gains the dissolve and why only at-once draws (0432 point 4).
- `3d/src/draw_group.c` / `draw_group.h`: only if the at-once draw goes through a helper there that
  builds the object, a `float dither` it sets; else leave them.
- Shadows (`draw_shadows.c`) and the bounce keep `voe_3d_model_level`: leave them.
- `3d/tests/draw_levels.c`: new cases on the levelled sphere, distances from `levels[1].bound` and
  the projection's P[1][1] as `3d/tests/model_levels.c` computes them:
  4. the row where s is midway through level 1's band: its count C4;
  5. the row where s is just below the band's low end: its count C5, and C4 − C5 at least the
     sphere's level-0 triangles (its index count ÷ 3, as the test builds it);
  6. case 4's distance with the row's `fade` 0.5: at most C5 (held, not dissolved).
  Header lists them. `3d/tests/tests.md`: the entry names the dissolve.
- `3d/src/src.md`: the `draw_models.c` entry names the dissolve in a phrase.

## Done when
The test `3d/draw_levels` passes after the folder's build, with cases 4 to 6 in it, and
`3d/draw_system` and `3d/draw_model_fade` still pass.
