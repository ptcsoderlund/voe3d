# 48 — A flat triangle gives no edges
folder: 3d
after: none
decisions: 0168

## Change
Bug 05: `walk_edges` in `3d/src/shape_geometry.c` (about line 134) normalizes every triangle's
cross of two edges; a triangle with no area (two corners on one point, three on one line) has a
cross of nought and `voe_math_float3_normalize` asserts. Every model load goes through
`voe_3d_shape_geometry_build` (`3d/src/models.c` `build_shape`), so this one fix covers the
editor's drop, a scene's open, Play and the game. Fix it here, once.

- `3d/src/shape_geometry.c` — `walk_edges`: before normalizing, a triangle whose cross has
  `voe_math_float3_length` not above nought (the test `normalize` asserts on) is skipped: it
  starts and fills no edge, in both walks (the count and the fill go through the same function,
  so they stay equal). An edge only a skipped triangle reached is then no edge; a real
  triangle's edge whose other side was flat is a boundary edge, as the header already describes.
  Header: a paragraph that a triangle with no area has no facing, so it gives no edges; that such
  triangles are common in exported models and not a fault; and the boundary-edge effect on the
  silhouette.
- `3d/include/3d/shape_geometry.h` — `voe_3d_shape_geometry_build`'s comment: triangles with no
  area are taken and give no edges, they do not assert. The file comment's line saying model
  triangles are "a later card" goes (they are built here today).
- `3d/src/src.md` — the `shape_geometry.c` entry adds that flat triangles are skipped; keep it
  within the entry cap.
- `3d/tests/shape_geometry.c` — new `static void flat_triangles_give_no_edges(void)` called from
  `main`: a geometry of one real triangle, one with two corners on one point and one with three
  corners on a line (its own small vertex and index arrays, on the test's arena) is built with
  `voe_3d_shape_geometry_build` without asserting; its `edge_count` is 3 (the real triangle's
  three, each a boundary edge); every edge's `left` and `right` are unit length. A second build
  of the two flat triangles alone gives `edge_count` 0. The file header's claims gain this one.
- `3d/tests/tests.md` — the `shape_geometry.c` entry names the flat triangles; within the cap.

Not to change: `pick.c` (its `PARALLEL` guard), `outline.c` (`DEGENERATE`), `simplify.c` and
`measure.c` (both test the length first) and `model_bake.c` (normals normalized only when not
nought) already take flat triangles.

## Done when
`ctest --test-dir build/debug -R '^3d/(shape_geometry|models|pick|outline)$'` passes with
`flat_triangles_give_no_edges` in `3d/tests/shape_geometry.c`.

The human's, in the editor on the lumberhill project:
1. Drag `spruce_ish` from the Assets panel into the scene view: it is placed, no crash. Save,
   close and reopen: the scene opens with it.
2. Pick it in the foliage brush with another tree and a rock and paint a slope (090 step 1).
