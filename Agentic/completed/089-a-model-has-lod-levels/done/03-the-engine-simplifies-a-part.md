# 03 — The engine simplifies a part's triangles
folder: 3d
after: none
decisions: 0168, 0425

## Change
0425 point 2: an in-house simplifier, internal to `3d` (rule 5: no meshoptimizer). New files.

- `3d/src/simplify.h` (new): header comment carrying 0425 point 2 as the reader needs it: half-edge
  collapse onto an existing vertex, cheapest first by area-weighted quadrics plus border planes; the
  cost a weighted mean squared distance, so its root is metres; topology welded by exact position;
  a seam position (vertices differing in normal, uv or colour complement) never moves; a border
  position moves only along a border edge; a collapse turning a remaining triangle past 90° refused;
  everything on the caller's scratch, rewound; no recursion. Declares
  `uint32_t voe_3d_simplify(voe_base_arena *scratch, const voe_render_vertex *vertices,
  uint32_t vertex_count, const uint32_t *indices, uint32_t index_count, uint32_t target,
  float max_error, uint32_t *out, float *error)`: writes at most `index_count` indices into `out`
  (each an index into `vertices`, whole triangles, none degenerate), stops when at most `target`
  indices remain or the next collapse's root cost passes `max_error`, sets `*error` to the root of
  the largest cost taken (nought when none), returns the count written.
- `3d/src/simplify.c` (new): the above. A binary heap of edge costs with a stamp per position, so a
  stale entry is skipped when popped. A collapsed position's corners take the vertex of the target
  that the collapsed edge's triangle names, so uv and colour stay those of a real vertex.
- `3d/src/src.md`: entries for both files, one sentence each.
- `3d/tests/simplify.c` (new), a plain program as the folder's others, building its meshes in code:
  1. a flat 32 × 32-quad grid to an eighth: at most the target, error under 1e-5, the four corners
     still used, every index below the vertex count, no degenerate triangle;
  2. a UV sphere of 32 × 16 to a half: at most the target, error above nought and under a tenth of
     the radius, every triangle still facing out (its normal · its centroid above nought);
  3. that sphere with `max_error` 1e-6: stops well above the target, at most index_count;
  4. the grid with its middle column of positions duplicated as a uv seam: every seam vertex's
     position is still used by some triangle.
  The header lists the four.
- `3d/tests/tests.md`: the `simplify.c` entry.

## Done when
The test `3d/simplify` passes after the folder's build.
