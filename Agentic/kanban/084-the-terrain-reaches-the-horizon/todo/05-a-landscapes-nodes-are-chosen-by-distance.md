# 05 — A landscape's nodes are chosen by their distance from the eye
folder: 3d
after: 01
decisions: 0168, 0379, 0395, 0396

## Change
0396 points 3 and 4, the CPU half: a min/max pyramid and the selection. CPU only, no device.
Nothing else in the folder changes; the chunk calls stay until card 06.

- `3d/include/3d/landscape.h` (header points: the node grid, ranges and morph, why neighbours
  differ by at most one level, that there is no frustum test until 093):
  - `VOE_3D_LANDSCAPE_NODE_QUADS 32`: quads a node side; a level-L node steps 2^L cells.
  - `voe_3d_landscape_node`: the node's corner x and z in the grid's own space, side in metres,
    lowest and highest height within it, level, morph start and end in metres.
  - `voe_3d_landscape_lod`: the level count and, per level, each node's lowest and highest height
    (arena memory). Levels run until one node of 32·2^L cells covers the grid; nodes past the
    grid's edge hold nothing and are never chosen.
  - `voe_3d_landscape_lod voe_3d_landscape_lod_build(const voe_assets_landscape *landscape,
    voe_base_arena *arena)`.
  - `void voe_3d_landscape_lod_update(voe_3d_landscape_lod *lod, const voe_assets_landscape
    *landscape, voe_3d_landscape_rect heights)`: the nodes over a changed rect refreshed, leaves
    from the heights and each level above from the one below.
  - `void voe_3d_landscape_lod_box(const voe_3d_landscape_lod *lod, const voe_assets_landscape
    *landscape, voe_math_float3 *min, voe_math_float3 *max)`: the grid's box from the root, the
    same as `voe_3d_landscape_box` without walking every height.
  - `uint32_t voe_3d_landscape_select(const voe_3d_landscape_lod *lod, const voe_assets_landscape
    *landscape, voe_math_float3 eye, voe_3d_landscape_node *nodes, uint32_t capacity)`: `eye` in
    the grid's space; walks from the root, a node split when its box is within its children's
    range and there is room for four more, else kept; how many written. Ranges: level 0's twice a
    leaf's diagonal, each next doubling; morph over the last 30% of a node's range.
- `3d/src/landscape.c` holds them if it stays under ~500 lines (it is 379); else a new
  `3d/src/landscape_lod.c` with its header comment, listed in `3d/src/src.md`.
- `3d/include/3d/3d.md` and `3d/3d.md`: the landscape entry names the selection.
- New `3d/tests/landscape_lod.c` (entry in `3d/tests/tests.md`), on a 2048-cell 4096 m grid with
  hills, eyes on a 9 × 9 lattice at two heights:
  - the chosen nodes cover the square exactly once (areas sum to size², no two overlap);
  - two nodes sharing an edge differ by at most one level;
  - never more than `capacity`, and capacity 4 still covers the square;
  - every node's low and high bound every height inside it;
  - after a brush stamp, `_lod_update` gives the same pyramid as a fresh build;
  - `_lod_box` equals `voe_3d_landscape_box`.

## Done when
`3d/tests/landscape_lod.c` passes.
