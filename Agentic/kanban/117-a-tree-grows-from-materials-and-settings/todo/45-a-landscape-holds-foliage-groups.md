# 45 — A landscape holds foliage groups
folder: assets
after: 44
decisions: 0168, 0520, 0521, 0516, 0517, 0511

## Change
Kinds become groups of glbs in the struct and the file (0521 points 1, 2, 4). This changes assets' public
surface; cards 47 to 51 move its users.

`assets/include/assets/landscape.h`:
- `VOE_ASSETS_LANDSCAPE_KINDS` and `_KIND_PATH` become `_GROUPS` 16, `_GROUP_GLBS` 8, `_GLB_PATH` 128;
  `_SHARE_MAX` 10, `_SHARE_DEFAULT` 1; the kind size limits are the glb's.
- The look loses `seed` and gains `uint8_t share`; its default share is 1.
- New `voe_assets_landscape_group`: `glbs[_GROUP_GLBS][_GLB_PATH]`, `looks[_GROUP_GLBS]`, `glb_count`,
  `uint8_t seed` (stored as the look's seed was: +1, 0 meaning the group's index).
- The landscape's kinds, looks and kind count become `groups[_GROUPS]` and `group_count`; grows are by
  group; a copy's `kind` becomes `group`.
- `_kind_size` becomes `_glb_size(l, group, glb)`; `_kind_seed` becomes `_group_seed(l, group)`;
  `_kind_remove` becomes `_group_remove`, renumbering copies and grows as it does now. `_seed_new` stays.

`git mv assets/src/landscape_kinds.c assets/src/landscape_groups.c` and the same for its test; the calls
above live there.

`assets/src/landscape_foliage.c`, `landscape_foliage.h`, `landscape.c`: read and write `[Foliage]` as
0521 point 1 spells it. The old `kind<K>` spelling reads, lenient and strict, as one-glb groups; both
spellings in one file, a share past 10, or a path twice in one group is malformed, handled as other
malformed foliage values are. The writer writes group keys only.

`assets/src/landscape_keys.c`: documented key rows for `glb<G>_<J>`, each per-glb look key, `share<G>_<J>`,
`seed<G>`, `grow<L>_<G>`; the `kind<K>` rows stay, saying they are read only, as one-glb groups.

Tests: the existing landscape tests (`assets/tests/landscape*.c`) build and pass on the new names, their
expectations unchanged. New tests in `assets/tests/landscape_groups.c`:
- an old-spelling text reads as one-glb groups with the same looks, seeds and copies;
- two groups of three and one glbs, with shares 0 and 3 and an off-default size, write and read back equal,
  and the written text has no `kind` key;
- both spellings in one text, and a share of 11, are malformed under the strict read.

Header points: groups and glbs, the share, the old spelling. Entries in the folder's `.md` files.

## Meaning
- Holds (0520): every saved project opens with each kind as a group of its one glb.

## Done when
`cmake --build --preset debug --target voe_assets` exits 0, and
`ctest --test-dir build/debug -R '^assets/landscape'` exits 0.
