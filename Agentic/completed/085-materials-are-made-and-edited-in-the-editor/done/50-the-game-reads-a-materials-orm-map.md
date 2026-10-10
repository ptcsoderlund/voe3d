# 50 — The game reads a material's ORM map
folder: game
after: 49
decisions: 0168, 0400

## Change
The renames of cards 48 and 49 in `game`; behaviour unchanged.

- `game/src/models_materials.c` — the values' `colormap`, `normalmap`, `ormmap`; the ORM map read
  into `maps.orm`. Header: "roughness map" becomes the ORM map, if it says so.
- `game/include/game/materials.h` — the header's example initializer uses `.colormap`.
- `game/tests/models.c` — `.colour_map` becomes `.colormap`.
- `game/src/src.md` — only if a line names the old names.

## Done when
`ctest --test-dir build/debug -R '^game/models$'` passes, and
`grep -rn 'colour_map\|normal_map\|roughness_map\|maps\.roughness' game` prints nothing.
