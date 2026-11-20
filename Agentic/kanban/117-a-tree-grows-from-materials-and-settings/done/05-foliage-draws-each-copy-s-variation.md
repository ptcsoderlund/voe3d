# 05 — Foliage draws each copy's variation
folder: 3d
after: 04
decisions: 0168, 0515, 0513, 0508, 0495, 0500

## Change
`3d/src/draw_foliage.c`. Where a kind's entry is looked up (`voe_3d_models_find(models, grid->kinds[k])`):
- keep the lookup once per kind;
- per copy, `voe_3d_models_variation(models, entry, k, copy.x, copy.z)` gives the entry the copy draws;
- the group key's entry is the variation's.

Grown copies (layer growth, 0476) take the pick from their own x and z in millimetres, as a painted copy
stores them.

Do the same wherever else under `3d/src/` a foliage kind's model is looked up to draw, shade or cast
shadows; grep for `kinds[`. LOD memory stays keyed by entity, kind, x and z.

Update the header comments of each changed file. In `3d/tests/draw_foliage.c`, add a case if its header
shows it can make a species entry. Otherwise the build and the existing tests are the proof.

## Meaning
- Precog (believed): foliage cost is overdraw; the depth EQUAL needs a shared vertex entry. A grown
  variation is an ordinary model entry, so both hold unchanged.

## Done when
`ctest --test-dir build/debug -R '^3d/draw_foliage$'` exits 0.
