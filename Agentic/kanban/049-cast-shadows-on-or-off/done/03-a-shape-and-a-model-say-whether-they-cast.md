# 03 — A shape and a model say whether they cast
folder: 3d
after: 01, 02
decisions: 0168, 0324

## Change
0324 points 1–2 for meshes; nothing reads the flag yet (card 04 does). Read
`3d/include/3d/shape_component.h`, `3d/include/3d/model_component.h`,
`3d/src/shape_component.c`, `3d/src/model_component.c`, the header of
`3d/include/3d/shape_system.h`, `3d/tests/shape.c`,
`3d/tests/model_component.c`, `3d/include/3d/3d.md`, `3d/src/src.md`,
`3d/tests/tests.md`.

- `shape_component.h`: `VOE_3D_SHAPE_FIELDS` gains `cast_shadows` (`bool`,
  BOOL) after `colour`. Header point: false leaves the shape out of every
  shadow and bounce map while it is still drawn, lit and shadowed (0301);
  true by default and in a file without it; zero is false, so a row built
  from a literal names it.
- `model_component.h`: `VOE_3D_MODEL_FIELDS` gains `cast_shadows` (`bool`,
  BOOL) after `path`; the same header point, for every part of the model.
- `shape_component.c` and `model_component.c`: the default rows cast.
- If the shape system's drain or the model drain copies fields one by one
  rather than the whole row, it carries the new field too.
- `3d/tests/shape.c` and `3d/tests/model_component.c`: the default row
  casts; an intent with `cast_shadows` false lands; with descriptions
  compiled in, the description names `cast_shadows` as BOOL. Fix any
  size or field-count check the new field breaks.
- `3d/include/3d/3d.md` (shape_component.h's and model_component.h's
  entries gain whether it casts), `src/src.md`, `tests/tests.md` where
  their entries no longer say what the file does.

## Done when
`ctest --test-dir build/debug -R '^3d/(shape|model_component)$'` passes with
the cast-shadows checks.
