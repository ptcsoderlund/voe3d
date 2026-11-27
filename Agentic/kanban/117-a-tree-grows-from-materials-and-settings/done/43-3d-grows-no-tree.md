# 43 — 3d grows no tree
folder: 3d
after: 42, 37
decisions: 0168, 0520, 0517

## Change
Take out of `3d/` what these commits added, seen with `git show <hash> -- 3d`: card 02 `9b0b3df5`, 03
`3859afc3`, 04 `8d41d784`, 05 `a2e1e6dd`, 23 `8dfc6c0c`, 33 `06ff80de`, and card 27 `53a1cccd`'s parts in
`models_trees.c` and `tree_grow.c`. Card 27's kind seed in `foliage_grow`, `models_foliage` and
`draw_foliage` stays (0520: the seed becomes the group's). Later cards' changes to the same files stay;
card 37 split two test files since.

- Delete `3d/include/3d/tree_grow.h`, `3d/src/tree_grow.c`, `tree_mesh.c`, `tree_joint.c`, `tree_joint.h`,
  `models_trees.c`, and `3d/tests/tree_grow.c`, `tree_joint.c`, `model_prepare_grown.c`.
- `3d/include/3d/model_prepare.h`, `3d/src/model_prepare.c`: `voe_3d_model_prepare_grown` goes.
- `3d/include/3d/models.h`, `3d/src/models.c`, `3d/src/models_landscape.c`: `VOE_3D_TREE_VARIATIONS`, an
  entry's grown and variation fields, `_put_grown`, `_species_set`, `_variation`, the `assets/tree.h`
  include and the grown case go.
- `3d/src/draw_foliage.c`, `draw_foliage.h`: the per-variation table goes; each copy draws its kind's own
  entry, as before card 05.
- `3d/tests/models.c` and the draw_foliage tests: species and variation tests go.
- Entries in `3d/3d.md`, `3d/include/3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md`.

`voe_3d_tree_pick`'s hash is ported to assets by card 46 from `git show 7e86cb12:3d/src/tree_grow.c`.

## Done when
`cmake --build --preset debug --target voe_3d` exits 0, and
`git grep -nE 'voe_3d_tree_|VOE_3D_TREE_|assets/tree\.h|species|models_variation|prepare_grown' -- 3d`
prints nothing.
