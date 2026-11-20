# 03 — A grown model prepared in memory, with measured levels
folder: 3d
after: 02
decisions: 0168, 0514, 0515, 0495, 0500

## Change
`3d/include/3d/model_prepare.h` and `3d/src/model_prepare.c`: add
`voe_3d_model_prepare_grown(arena, <the prepared out, as voe_3d_model_prepare takes it>,
const voe_assets_model *model, voe_base_error *error)`. It is what `voe_3d_model_prepare` does after
its glb parse, with no glb bytes and no levels file:
- own levels from the `_LOD<n>` nodes, through `3d/src/model_bake.h`;
- each level's error measured both ways against level 0, through `3d/src/measure.h`, as an imported
  model's own levels are.

Read the header of `model_prepare.h` for prepare's signature and its prepared struct. Share the code that
follows the parse; do not copy it. A model with no pictures prepares with none; its parts get their
materials later, from paths.

Header comment: when to use which entry point, and that the grower's output is prepared here.

New test `3d/tests/model_prepare_grown.c`, using 02's grower with default values:
- it prepares with `VOE_3D_TREE_LEVELS` levels;
- the errors rise from level to level;
- `voe_3d_model_levels_bounds` on them gives bounds that rise.

Add its entry to `3d/tests/tests.md`.

## Meaning
- Scout: there is no public in-memory prepare; prepare asserts the glb bytes; the bake reads
  `voe_assets_model`; the bounds functions are pure arithmetic.
- Precog (believed): LOD bounds are made only in the store.

## Done when
`ctest --test-dir build/debug -R '^3d/model_prepare_grown$'` exits 0.
