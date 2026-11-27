# 49 — Each copy draws its picked glb
folder: 3d
after: 48
decisions: 0168, 0520, 0521, 0517, 0469

## Change
`3d/src/draw_foliage.c`, `3d/src/draw_foliage.h`: each copy draws the glb `voe_assets_landscape_glb_pick`
gives for its group at its x and z: that glb's model and LODs, its shadows and its blend. Grown copies the
same.

`3d/include/3d/lods_kept.h`, `3d/src/lods_kept.c`: `voe_3d_lods_kept_copy_key(landscape, group, glb, x, z)`
so each glb keeps and swaps its own LODs; its caller in `draw_foliage.c` and `3d/tests/lods_kept.c`
follow.

New tests in the draw_foliage test files:
- a glb of share 0 in a two-glb group is never drawn;
- in a group whose second glb is shadowless, only that glb's copies are left out of the shadow cascades;
- two glbs' copies of one group keep apart LOD keys.

Header points: the picked glb is drawn, its own LODs. Entries in `3d/src/src.md`, `3d/tests/tests.md`.

## Meaning
- Scout: the variation pick hashed the kind's seed with the copy's x and z in mm and took `% count`.

## Done when
`ctest --test-dir build/debug -R '^3d/(draw_foliage|lods_kept)'` exits 0 after building `voe_3d` and
those test targets.
