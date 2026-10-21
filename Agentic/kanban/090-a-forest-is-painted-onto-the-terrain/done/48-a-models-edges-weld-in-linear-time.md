# 48 — A model's edges weld in linear time
folder: 3d
after: none
decisions: 0168, 0447, 0448

## Change
0448 point 6. Sampling the lumberhill start put about a third of its 9.7 s in the CPU edges'
weld, which compares every vertex with every representative before it; a model's own vertices
make that quadratic.

- `3d/src/shape_geometry.c` — `weld`: positions hashed into cells of
  `VOE_3D_SHAPE_GEOMETRY_WELD` a side; each vertex compared, by `same_position`, with the
  representatives already in its cell and the 26 around it, the lowest-numbered within the weld
  its representative, else itself, added to its cell. The same answer as today's walk, in time
  linear in the vertices. The table and its chains on the arena, given back before
  `voe_3d_shape_geometry_build` returns, as the edge table is. Header: the paragraph saying the
  weld is quadratic and that a model wants a grid becomes how the cells answer the same
  representative as the walk did; the "few thousand triangles" startup wording goes, a model's
  millions being built here too.
- `3d/src/src.md` — the `shape_geometry.c` entry: the weld through cells.
- `3d/tests/shape_geometry.c` — a new check: a flat grid of 512 × 512 quads, each quad with
  four vertices of its own (as a cube's corners are) and every copy of a corner moved by less
  than the weld (3e-5 m) in a different direction, so copies straddle a cell's side, built with
  `voe_3d_shape_geometry_build`: its edge count is 2·512·513 + 512² (787 456), every edge's
  `left` and `right` the grid's up. A quadratic weld of its million vertices would not finish
  inside the test's time.
- `3d/tests/tests.md` — the `shape_geometry.c` entry names the check.

## Done when
`ctest --test-dir build/debug -R '^3d/(shape_geometry|pick|outline|models)$'` passes with the new
check.
