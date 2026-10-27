# 05 — A placed model has a Blend with ground switch
folder: 3d
after: 02, 04
decisions: 0168, 0481

## Change
0481 point 4, the model's half: two described fields, so save, undo, the Inspector's rows and the
cook carry them with no code of their own (as `fade` was added). Nothing draws them yet (card
06).

- `3d/include/3d/model_component.h` — `VOE_3D_MODEL_FIELDS` gains, after `fade`,
  `F(bool, blend_with_ground, BOOL)` and `F(float, blend_height, FLOAT32)`. A header paragraph:
  BLEND WITH GROUND takes on the painted ground near where the model meets it, fading into its
  own material `blend_height` metres up (0481 points 1, 3); off by default, in a file without it
  and in a literal; the height 0.5 in the default row and a file without it, a literal's 0
  blending nothing; the draw clamps it to 0.05–10 m. `_register`'s comment names the default
  row's new values.
- `3d/src/model_component.c` — the default row registered with `blend_with_ground` false and
  `blend_height` 0.5.
- `3d/3d.md` and `3d/src/src.md` — the `model_component` entries name the switch and height.
- `3d/tests/model_component.c` — its default-row and read-back claims cover both fields: the
  default row reads false and 0.5; a row with true and 2 submitted reads back so. Its header's
  claims follow.
- `3d/tests/tests.md` — the `model_component.c` entry follows.

## Done when
`ctest --test-dir build/debug -R '^3d/model_component$'` passes.
