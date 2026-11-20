# 01 — The species file: read, written, checked, documented
folder: assets
after: none
decisions: 0168, 0515, 0511, 0512, 0498, 0509

## Change
New `assets/include/assets/tree.h` and `assets/src/tree.c`. Read the header of
`assets/include/assets/material.h` and mirror it: lenient reader, strict reader, writer, default,
key table.
- `voe_assets_tree`: `bark` and `leaf` paths (each with the room a material path has), floats
  `height trunk crown branching reach angle droop crooked leaf_size leaf_density`, and `variations` as
  an unsigned integer.
- A MIN, MAX and DEFAULT constant per key, from 0515's table. These are the ones the Inspector and the
  key table share.
- The functions:
  - `voe_assets_tree_default(void)`.
  - `voe_assets_tree_read`: lenient. A missing key reads as its default, an out-of-range one is
    clamped, an unknown one is skipped.
  - `voe_assets_tree_read_strict`: an unknown key or section, or an out-of-range value, is an error
    naming it, as material's strict reader does.
  - `voe_assets_tree_write`.
  - `voe_assets_tree_keys(uint32_t *count)`: the key table (0512's row type).
- Path helpers:
  - `voe_assets_tree_path_is(path)`: true for a path ending `.tree` in any case, or `.tree#<digits>`.
  - `voe_assets_tree_variation_path(out, room, path, v)`: writes `<path>#<v>`, false when it will not fit.

Header comment points:
- an example file;
- this file is all a species keeps on disk (0510);
- what each key means and its unit, angle in degrees;
- the variation path form, `#` never part of a made file's name.

`assets/include/assets/landscape.h` header: the "KINDS ARE QUOTED .glb PATHS" point now also lets a kind
name a `.tree` species. No code changes in landscape.c.

Index entries for `tree.h`, `tree.c` and `tests/tree.c` in `assets/assets.md`, `assets/src/src.md` (if
present) and `assets/tests/tests.md`.

New test `assets/tests/tree.c`:
- the default written and read back is equal, field by field;
- the lenient reader clamps an out-of-range value and defaults a missing key;
- the strict reader refuses an unknown key, an unknown section and an out-of-range value;
- `path_is` handles `a.tree`, `A.TREE`, `a.tree#3`, `a.tree#`, `a.glb` and `a.tree.glb`;
- the key table has one row per key.

## Done when
`ctest --test-dir build/debug -R '^assets/tree$'` exits 0.
