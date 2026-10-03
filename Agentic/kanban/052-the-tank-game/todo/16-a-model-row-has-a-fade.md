# 16 — A model row has a fade
folder: 3d
after: none
decisions: 0168, 0336

## Change
0336 point 1; nothing draws by it yet (card 18 does). Read `3d/include/3d/model_component.h`,
`3d/src/model_component.c`, `3d/tests/model_component.c`, `3d/include/3d/3d.md`,
`3d/tests/tests.md`.

- `model_component.h`: `VOE_3D_MODEL_FIELDS` gains `fade` (`float`, FLOAT32) after
  `cast_shadows`. A header paragraph: 0 draws as ever, 1 is gone, between is see-through by that
  much; 0 by default, in a file without it and in a literal, so nothing written before it changes;
  game code fades a thing through the intent.
- `model_component.c`: the default row's `fade` is 0. If the drain copies fields one by one
  rather than the whole row, it carries `fade` too.
- `3d/tests/model_component.c`: the default row's `fade` is 0; an intent with `fade` 0.5 lands
  and reads back after a run; with descriptions compiled in, the description names `fade` as
  FLOAT32. Fix any size or field-count check the new field breaks.
- `3d/include/3d/3d.md` (the `model_component.h` entry) and `3d/tests/tests.md` (the
  `model_component.c` entry): each gains the fade, a phrase.

## Done when
`ctest --test-dir build/debug -R '^3d/model_component$'` passes with the fade checks.
