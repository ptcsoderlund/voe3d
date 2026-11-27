# 46 — A group picks a glb by share, and its glbs are added, replaced and removed
folder: assets
after: 45
decisions: 0168, 0520, 0521, 0517

## Change
`assets/include/assets/landscape.h` and `assets/src/landscape_groups.c`:
- `uint32_t voe_assets_landscape_glb_pick(const voe_assets_landscape *, uint32_t group, int32_t x_mm,
  int32_t z_mm)`: 0521 point 3. Port the hash of `voe_3d_tree_pick` from
  `git show 7e86cb12:3d/src/tree_grow.c` (near line 194), keyed by `_group_seed`; take it modulo the
  shares' total and walk the glbs in order. All shares 0 picks as all 1. Nothing is stored.
- `bool voe_assets_landscape_group_add(l, const char *path)`: a new group holding that glb, default look,
  a seed from `_seed_new`; false at `_GROUPS`.
- `bool voe_assets_landscape_glb_add(l, uint32_t group, const char *path)`: appends a glb with the default
  look; false at `_GROUP_GLBS` or when the group holds that path.
- `bool voe_assets_landscape_glb_set(l, group, glb, path)`: replaces the path, keeping the look; false
  when another glb of the group holds it.
- `void voe_assets_landscape_glb_remove(l, group, glb)`: later glbs move down one; copies stay, as their
  pick is live. Asserts the group holds more than one glb.

New `assets/tests/landscape_pick.c`:
- one glb always picks 0;
- six glbs of share 1 in a group whose `_group_seed` is 3 pick 2 at (1733, -911), as
  `voe_3d_tree_pick(3, 1733, -911, 6)` did;
- four glbs of share 1 over 10000 positions each come up within 5% of a quarter;
- shares 3, 1, 1, 1: the first comes up 50% ± 3%;
- a glb of share 0 never comes up; all shares 0 spread as all 1;
- the same position picks the same glb twice;
- adds refused at the caps and for a repeated path; a refused replace; a removal moves later glbs down.

Header points: the pick is live and why, the refusals. Entries in `assets/include/assets/assets.md`,
`assets/src/src.md`, `assets/tests/tests.md`.

## Meaning
- Scout: the variation pick hashed the kind's seed with the copy's x and z in mm and took `% count`.

## Done when
`ctest --test-dir build/debug -R '^assets/landscape_pick'` exits 0 after building `voe_assets` and its
test targets.
