# 48 — The material file's maps are colormap, normalmap and ormmap
folder: assets
after: none
decisions: 0168, 0400

## Change
0400: the `.material` file's three map keys, and the struct's fields that hold them, are
`colormap`, `normalmap` and `ormmap`; `roughness_map` is gone, replaced by the ORM map.

- `assets/include/assets/material.h` — in `voe_assets_material_file`, `colour_map` becomes
  `colormap`, `normal_map` becomes `normalmap`, `roughness_map` becomes `ormmap`. Header: the example
  file uses the new keys; points that `ormmap` is one picture of occlusion in red, roughness in green,
  metal in blue (0400), and what a file with the old keys reads as.
- `assets/src/material.c` — the read's three keys and the write's three keys and fields follow.
- `assets/tests/material.c` — every field use follows; add a check that text with `ormmap="a.png"`
  reads into `ormmap`, and that the written text contains `ormmap=`.
- `assets/assets.md`, `assets/src/src.md`, `assets/tests/tests.md` — any line naming the old keys.

`3d`, `game` and `editor` name the old fields; cards 49, 50 and 51 rename those. Do not touch them.

## Done when
`ctest --test-dir build/debug -R '^assets/material$'` passes, and
`grep -rn 'colour_map\|normal_map\|roughness_map' assets` prints nothing.
