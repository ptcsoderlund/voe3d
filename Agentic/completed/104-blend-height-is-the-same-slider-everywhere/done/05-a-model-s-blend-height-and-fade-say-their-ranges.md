# 05 — A model's blend height and fade say their ranges
folder: 3d
after: 03, 04
decisions: 0168, 0481, 0494, 0498

## Change
Give 3d's bounded component fields a range (base/describe.h's ranges list, card 01). Each header
gets a `..._RANGES(R)` list and `VOE_BASE_DESCRIBE_STRUCT` becomes
`VOE_BASE_DESCRIBE_STRUCT_RANGED`; struct, scene text and every check unchanged.

- `3d/include/3d/model_component.h`: `blend_height` 0.05–10, unit `m`; `fade` 0–1, unit empty.
  Name the ends `VOE_3D_MODEL_BLEND_HEIGHT_LEAST` (0.05f) and `VOE_3D_MODEL_BLEND_HEIGHT_GREATEST`
  (10.0f), metres, and write the range with them; the editor's foliage row uses them too (card 07). `voe_3d_draw_ground_fill` in `3d/src/draw_ground.c` clamps `reach` to
  0.05–10 m (its header `3d/src/draw_ground.h` says so): make it clamp to these constants, and its
  header comment name them. Header points: the
  range is the one the draw clamps to, so the slider and the draw agree; a foliage kind's Blend
  height is shown over this same range (0494 point 2, 0498 point 3).
- `3d/include/3d/emitter_component.h`: `alpha_start` and `alpha_end` 0–1, unit empty. Header
  point: alpha is an opacity from 0 to 1.

Tests:
- `3d/tests/model_component.c`: `voe_base_ranges_find` gives `blend_height` 0.05–10 `m` and `fade`
  0–1, NULL for `cast_shadows`.
- `3d/tests/emitter_component.c`: both alphas 0–1, NULL for `rate`.

`3d/tests/tests.md`: those two entries also say their ranges. `3d/3d.md`: the
`model_component.h` entry says its blend height's range.

## Done when
`ctest --test-dir build/debug -R '^3d/(model_component|emitter_component|draw_ground)$'` passes
with the new checks.
