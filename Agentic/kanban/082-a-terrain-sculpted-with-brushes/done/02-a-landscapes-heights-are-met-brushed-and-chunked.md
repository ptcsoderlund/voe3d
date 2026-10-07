# 02 — A landscape's heights are met, brushed and chunked
folder: 3d
after: 01
decisions: 0168, 0379

## Change
The arithmetic on a `voe_assets_landscape` grid (assets/landscape.h) the store, the pick and the editor
share (0379 points 2–3). CPU only, no device.

- New `3d/include/3d/landscape.h`:
  - `VOE_3D_LANDSCAPE_CHUNKS` (4, per side: 16 chunks, one per model part).
  - `voe_3d_landscape_rect` — `uint32_t x0, z0, x1, z1`, height indices, end exclusive; empty when
    x0 ≥ x1 or z0 ≥ z1.
  - `voe_3d_brush_kind` — RAISE, LOWER, SMOOTH, FLATTEN. `voe_3d_brush` — kind, `radius`, `strength`,
    `softness`, `target` (flatten's height).
  - `float voe_3d_landscape_height(const voe_assets_landscape *, float x, float z)` — bilinear, the
    point clamped onto the grid.
  - `bool voe_3d_landscape_ray(const voe_assets_landscape *, voe_math_float3 origin,
    voe_math_float3 direction, float *distance)` — the ray in the grid's own space: stepped half a cell
    at a time across the grid's box, the crossing bisected; false when it never goes below the ground.
  - `voe_3d_landscape_rect voe_3d_landscape_brush(voe_assets_landscape *, const voe_3d_brush *, float x,
    float z, float seconds, voe_base_arena *scratch)` — one stamp at (x, z), seconds clamped to 0.1, by
    0379 point 3's weight and rates; smooth reads a copy of the rect in `scratch` (rewound); answers the
    rect it changed.
  - `voe_3d_landscape_mesh` — `voe_render_vertex *vertices; uint32_t vertex_count; uint32_t *indices;
    uint32_t index_count`. `voe_3d_landscape_mesh voe_3d_landscape_chunk(const voe_assets_landscape *,
    uint32_t chunk, voe_base_arena *arena)` — chunk `cz·4 + cx`'s (cells/4 + 1)² vertices: position,
    normal by central differences, uv the grid's 0..1; triangles wound counter-clockwise seen from +Y.
  - `voe_3d_landscape_rect voe_3d_landscape_chunks(const voe_assets_landscape *, voe_3d_landscape_rect)`
    — the chunks (as chunk x and z ranges) a height rect touches, a chunk's edge row counted in both.
  - `void voe_3d_landscape_box(const voe_assets_landscape *, voe_math_float3 *min, voe_math_float3 *max)`.
  - Header points: the grid's own space (0379 point 1); why a march and bisect and not a triangle walk;
    the weight and rates and why seconds are clamped; why chunks share their edge heights.
- New `3d/src/landscape.c` — carries those out.
- New `3d/tests/landscape.c` — `height_is_bilinear_between_four`, `ray_meets_a_raised_cell`,
  `ray_beside_the_grid_misses`, `raise_lifts_the_middle_most_and_the_rim_not_at_all`,
  `smooth_lowers_a_spike`, `flatten_moves_toward_the_target`, `chunk_has_its_quads_wound_up`,
  `chunks_of_a_rect_on_an_edge_are_both`.
- `3d/3d.md` gains the `landscape.h` entry; `3d/src/src.md` and `3d/tests/tests.md` theirs; each a phrase.

## Done when
`ctest --test-dir build/debug -R '^3d/landscape'` passes with the eight tests above.
