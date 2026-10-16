# 69 — Two bakes are measured apart
folder: 3d
after: none
decisions: 0168, 0435, 0436

## Change
0435 point 3, 0436 point 6: a model's own LODs swap by their error, so the error of an own
level must be measured against level 0. This card adds the measure only; card 72 uses it.

- New `3d/src/measure.h` and `3d/src/measure.c`, internal to 3d:
  `float voe_3d_measure_apart(voe_base_arena *scratch, const voe_3d_model_bake *a,
  const voe_3d_model_bake *b)` — the largest distance, in the bakes' own metres, from any
  vertex of `a` to the nearest triangle of `b` and from any vertex of `b` to the nearest
  triangle of `a`: 0 for the same bake. Triangles are every part's, indices counted from the
  part's first vertex (`3d/src/model_bake.h`'s header). Positions are the vertices'
  positions as `voe_render_vertex` holds them.
- In `measure.c`: a uniform grid over the triangles' joint box, about one triangle per cell
  (cell count from the triangle count, at least one per axis), each triangle listed in every
  cell its box overlaps, on `scratch`; a nearest-triangle search that visits rings of cells
  outward from the vertex's cell and stops once the next ring is farther than the best found;
  the point-to-triangle distance by the closest-point regions (Ericson, Real-Time Collision
  Detection 5.1.5). `scratch` is rewound before returning. Asserts both bakes hold a triangle.
  Loops bounded by the grid's size (rule 14 in 0168).
- Header comment of `measure.h`: what is measured and why (own LODs' errors, 0435), that it is
  symmetric, the grid and its cost, and the arena use.
- `3d/src/src.md`: one-sentence entry for `measure.h`/`measure.c`.
- New `3d/tests/measure.c` (includes `"../src/measure.h"`, as `3d/tests/model_bake.c` reaches
  `model_bake.h`), bakes built by hand on an arena:
  - `same_bake_is_nought`: a unit quad against itself, 0.
  - `lifted_quad_is_its_lift`: a unit quad against the same quad 0.25 higher, 0.25 within 1e-6.
  - `small_inside_big_is_the_far_corner`: a 2×2 quad against a 1×1 quad in its corner, both
    flat: the big quad's far corner to the small quad, √2 within 1e-5, either order.
  - `grid_finds_far_triangles`: a 32×32 grid of quads against a copy with one vertex moved
    0.5 up: 0.5 within 1e-5.
  `3d/tests/tests.md`: one-sentence entry.

## Done when
The test `3d/measure` passes after the folder's build.
