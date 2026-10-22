# 32 — A model's made LODs walk LOD 1's vertices
folder: 3d
after: 25, 31
decisions: 0168, 0444

## Change
0444: a made level uploads only its indices, walking level 0's vertices through card 25's
`voe_render_geometry_create_sharing` (read its comment in `render/include/render/device.h`).
Own `_LOD<n>` levels keep vertices of their own, as today.

- `3d/src/models_levels.c` — a made level's part: `voe_render_geometry_create_sharing` over its
  level 0 part's geometry (the mirror in `levels[0]`) with the levels file's indices as they are,
  already counted from the part's first vertex; the gather of used vertices, the remap and the
  vertex copy on the scratch arena go. `voe_3d_models_levels_free` stays the one free of levels 1
  on. Header: the CONSTRAINTS paragraph on the gather goes; a paragraph says made levels share
  level 0's vertices (0444), so their free must come before level 0's geometries are destroyed.
- `3d/src/models.c` — wherever a part's geometries are destroyed and `voe_3d_models_levels_free` is
  called after (an entry's free, and a failed load giving back what it made), the levels' free
  comes first, since `render` asserts on a base destroyed while a sharer walks it. Header, if it
  names the order.
- `3d/src/models_store.h` — the levels paragraph, if it says how levels are uploaded.
- `3d/include/3d/models.h` — comments only: the levels paragraph says made levels walk level 0's
  vertices, not compacted copies; the room paragraph says `VOE_3D_MODELS_VERTICES` and `_INDICES`
  are the starting room a program adds, which `render` grows (0444), and that made levels add
  indices only.
- `3d/src/src.md` — the `models_levels.c` entry: no gather; levels share level 0's vertices.
- `3d/tests/models_levels.c` — new checks: every made level's part geometry has the same
  `voe_render_geometry_box` as its level 0 part (a sharer's box is its base's); a levelled sphere
  loads onto a headless device whose starting `vertices` and `indices` are below its level 0's,
  with every level made, and draws as the other cases do; clearing the store after it asserts
  nothing.
- `3d/tests/tests.md` — the `models_levels.c` entry names the new checks.

## Done when
`ctest --test-dir build/debug -R '^3d/(models_levels|models|draw_levels)$'` passes with the new
checks.
