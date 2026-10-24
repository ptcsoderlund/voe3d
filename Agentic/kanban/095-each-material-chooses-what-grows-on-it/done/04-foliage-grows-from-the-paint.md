# 04 — Foliage grows from the paint
folder: 3d
after: 02
decisions: 0168, 0442, 0473, 0476

## Change
0476 point 3 as CPU arithmetic: the copies a landscape's layers grow over an area, made from the
paint with no state. Nothing calls it yet; card 05 keeps them in the store.

- New `3d/include/3d/foliage_grow.h` and `3d/src/foliage_grow.c` —
  - `VOE_3D_FOLIAGE_GROWN`, the most grown copies a landscape holds: VOE_ASSETS_LANDSCAPE_COPIES.
  - `uint32_t voe_3d_foliage_grow(const voe_assets_landscape *landscape,
    voe_3d_landscape_rect rect, uint32_t kinds, voe_assets_landscape_copy *grown,
    voe_scene_transform *placed, uint32_t count)` — the new count. The area is the rect's points
    widened by one cell, in metres, half-open. Every copy in `grown[0..count)` of a kind in
    `kinds` (bit k) standing in the area is removed by moving the last in, `placed` alike; then,
    for each such kind, every kept lattice point in the area is appended, up to
    VOE_3D_FOLIAGE_GROWN, and placed by `voe_3d_foliage_place`. A kind no layer grows adds none.
  - The lattice, the draws and the keep are 0476 point 3's: spacing √(1000 / D) m from the grid's
    corner; each cell's point jittered inside it and its keep, scale, turn and lean drawn from a
    hash of (kind, cell x, cell z); kept where the keep draw is under d / D, d each layer's
    bilinear weight / 255 times its `grows`, summed (paint NULL is all layer 0); scale, turn and
    lean from the kind's look as a scatter draws them (card 02), kind k.
  - Header: what it is for; why no state (a remake over any area equals the whole remake there,
    and the game makes the editor's copies); why the lattice is at the most any layer asks (the
    keep is a share, never above one); the cost (every grown copy once, then the lattice cells
    in the area); the cap and that past it the result depends on the order of remakes.
- New `3d/tests/foliage_grow.c` — on 256 m, 64 cells, two layers:
  - all layer 1, layer 1 growing kind 0 at 40 tenths: the count is within 15 % of
    0.04 × 256²;
  - layer 1 painted on the west half only: no copy stands more than one cell east of it;
  - a second layer growing kind 0 at 20 painted on the east half: the east holds about half the
    west's, within 15 points of 50 %;
  - grow the whole, change the paint inside a rect, remake that rect: the copies, sorted by
    kind, x and z, equal a whole grow from nothing on the new paint;
  - remaking only kind 1 leaves kind 0's copies as they were;
  - on a 512 m grid with every layer growing two kinds at 500 tenths, the count stops at
    VOE_3D_FOLIAGE_GROWN.
- `3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md` — an entry each for the new files.

## Done when
The `3d/foliage_grow` test passes.
