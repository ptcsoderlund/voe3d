# 48 — The store paints and grows groups
folder: 3d
after: 46
decisions: 0168, 0520, 0521, 0516, 0517

## Change
3d moves to card 45's groups. This changes 3d's public surface; cards 50 and 51 move its users.

`3d/include/3d/models_foliage.h`, `3d/src/models_foliage.c`, `3d/src/models_landscape.c`,
`3d/include/3d/models.h` (the store's per-kind model entries become one per group and glb):
- `_kind_set` becomes `bool voe_3d_models_landscape_glb_set(models, path, group, glb, model)`: a group
  equal to `group_count` adds a group (`voe_assets_landscape_group_add`), a glb equal to its `glb_count`
  adds a glb (`_glb_add`), else it replaces the path (`_glb_set`); false when assets refuses.
- `_look_set(models, path, group, glb, look)`: a size or share change re-places the group's painted
  copies; a size, spread or share change remakes its grown copies.
- `_grow_set(models, path, layer, group, density)`.

`3d/include/3d/foliage.h`, `3d/src/foliage.c`, `3d/include/3d/foliage_grow.h`, `3d/src/foliage_grow.c`:
the brush's picked bits are per group; painting and growing place copies of a group, each copy's base size
and slope tilt from the glb `voe_assets_landscape_glb_pick` gives at its x and z (0521 point 3).

`3d/src/draw_foliage.c`, `draw_foliage.h`: only what the build needs; each copy draws its group's first glb
until card 49.

Existing tests that name kinds (`3d/tests/foliage.c`, `foliage_grow.c`, `models_landscape*.c`,
`draw_foliage*.c`, `bounce_foliage.c`, `lods_kept.c`) build on one-glb groups, expectations unchanged.
New test in `3d/tests/foliage.c`: a group of two glbs of base sizes 1 and 2; each painted copy's scale
follows its picked glb, and raising one glb's base size changes only the copies that pick it.

Header points: a group's copies and the live pick. Entries in `3d/include/3d/3d.md`, `3d/src/src.md`,
`3d/tests/tests.md`.

## Done when
`cmake --build --preset debug --target voe_3d` exits 0;
`ctest --test-dir build/debug -R '^3d/(foliage|models_landscape|draw_foliage|bounce_foliage|lods_kept)'`
exits 0 after building those test targets; and
`git grep -nE 'LANDSCAPE_KINDS|KIND_PATH|kind_count|kind_set|kind_seed' -- 3d` prints nothing.
