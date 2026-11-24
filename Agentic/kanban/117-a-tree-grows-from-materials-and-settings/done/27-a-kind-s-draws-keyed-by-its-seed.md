# 27 — A kind's draws are keyed by its seed
folder: 3d
after: 23, 26
decisions: 0168, 0517, 0515, 0476

## Change
Every draw keyed by a kind takes `voe_assets_landscape_kind_seed`, not its index (0517 point 4). For a
landscape without seed keys every seed is its index, so nothing drawn today moves.

- `3d/src/foliage_grow.c` and `3d/include/3d/foliage_grow.h`: the lattice point's hash mixes the kind's
  seed where it mixes `kind` now. The header's THE LATTICE and NO STATE points say seed.
- `3d/src/tree_grow.c` and `3d/include/3d/tree_grow.h`: `voe_3d_tree_pick`'s first parameter is renamed
  `seed`, the kind's seed; the hash is unchanged.
- `3d/src/models_trees.c` and `3d/include/3d/models.h`: `voe_3d_models_variation`'s `kind` parameter is
  renamed `seed` and passed on; its header and the models.h point naming the pick say seed.
- `3d/src/draw_foliage.c`: the variation is asked with the copy's kind's seed from its landscape. Its
  header point at line 39 says seed.
- `3d/src/models_foliage.c` and `3d/include/3d/models_foliage.h`:
  - `voe_3d_models_landscape_kind_set`: an added kind's look is the default with its seed
    `voe_assets_landscape_seed_new` (stored plus one);
  - `voe_3d_models_landscape_look_set`: the kind keeps its held seed whatever the look passed in
    holds; the equal-look check ignores it.

Signatures keep their types, so callers elsewhere still build. `lods_kept`'s copy key stays on the index:
it lives a frame.

New tests:
- `3d/tests/foliage_grow.c`: a landscape whose kind 0 has seed 1 grows, in the same rect and layer, the
  copies, x and z, that the same landscape with that kind at index 1 and no seed grows.
- `3d/tests/tree_grow.c`: `voe_3d_tree_pick(s, x, z, n)` is unchanged for seed s = old kind s, pinned on
  a few points before the rename.

## Done when
`ctest --test-dir build/debug -R '^3d/(foliage_grow|tree_grow|models|models_landscape)$'` exits 0 with
the new tests in it.
