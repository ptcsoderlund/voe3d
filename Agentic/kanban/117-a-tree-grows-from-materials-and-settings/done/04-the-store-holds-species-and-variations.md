# 04 — The store holds species and their variations
folder: 3d
after: 03
decisions: 0168, 0515, 0513

## Change
`3d/include/3d/models.h`, with the new code in a new `3d/src/models_trees.c` so that `3d/src/models.c`
(707 lines) stays small. Change `models.c` only where an existing call must treat a grown entry
differently.

`voe_3d_model_entry` gains:
- `grown`;
- `variation_count`;
- the variations, `VOE_3D_TREE_VARIATIONS` 16 of them, equal to `VOE_ASSETS_TREE_VARIATIONS_MAX`, held
  as the store refers to entries.

New calls:
- `voe_3d_models_put_grown(models, device, path, prepared, bark, leaf, error)`: `_put` at stamp 0, then
  every part wears `bark` when its source is material 0 and `leaf` when it is 1, and the entry is marked
  grown.
- `voe_3d_models_species_set(models, path, count)`: the entry at `path` is loaded with no parts, and its
  variations are the loaded entries `<path>#0`..`#count-1`. False when the store is full.
- `voe_3d_models_variation(models, entry, kind, x, z)`: `entry` itself unless it is a species; for a
  species, variation `voe_3d_tree_pick(kind, x, z, count)`, or NULL when that variation is not loaded.

Pure, in `3d/include/3d/tree_grow.h` and `3d/src/tree_grow.c`: `voe_3d_tree_pick(kind, int32_t x_mm,
int32_t z_mm, count)`, an integer mix of the three taken modulo count.

Changed behaviour:
- `voe_3d_models_parts_set` does nothing for a grown entry.
- `voe_3d_models_rename` moves `from#<v>` entries with `from`, and moves a species' variations with it.

Header comment: a GROWN TREES point (0515), covering the entry names, worn paths, species entry, pick
and rename. Index entries in `3d/src/src.md`.

Tests:
- `3d/tests/tree_grow.c`: the pick is the same for the same inputs, and 6 variations over 10 000 grid
  copies each take between 12 % and 22 %.
- If `3d/tests/models.c` makes a device (read its header), it also tests that a grown put wears the bark
  and leaf paths, that `parts_set` leaves them, that a species entry returns its variations, and that a
  rename moves `a.tree#0`.

## Done when
`ctest --test-dir build/debug -R '^3d/(tree_grow|models)$'` exits 0.
