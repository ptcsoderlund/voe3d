# 04 — A brush paints a layer
folder: 3d
after: 01
decisions: 0168, 0379, 0402

## Change
0402 point 3: the paint stamp, beside the sculpting stamps, CPU only.

- `3d/include/3d/landscape.h` —
  - `voe_3d_brush_kind` gains `VOE_3D_BRUSH_PAINT` after `_FLATTEN` (the editor indexes its four
    sculpt buttons by the first four values; keep them).
  - `voe_3d_brush` gains `layer`, the layer PAINT raises, below `layer_count`.
  - `voe_3d_landscape_brush` with PAINT stamps the paint instead of the heights: each point
    within the radius moves its layer's share up toward strength·weight by min(1, 8·s) of the
    gap (s clamped to 0.1 as the others), never down; the other seven scale down by what that
    leaves, rounded, the rounding given to layer 0 (to layer 1 when layer 0 is the one painted),
    so the eight bytes still sum to 255. A NULL `paint` is the caller's bug (assert): the store
    always holds one. Returns the rect of points whose bytes changed.
  - Header points: PAINT's rate and cap as 0402 point 3, why a light stroke stops at its
    strength, and the sum kept.
- `3d/src/landscape.c` — the paint stamp, using the same weight function as the others.
- `3d/tests/landscape.c` — new cases on a flat 8-cell grid with three layers and paint the test
  allocates as all layer 0: one PAINT stamp of layer 2 at strength 1 for 0.1 s at the centre
  raises the centre's layer 2 and lowers its layer 0, every point still sums to 255, a point past the radius is untouched;
  many stamps at strength 0.3 leave layer 2 at 0.3·255 ± 2 at the centre and never above; a
  stamp of layer 0 over that lowers layer 2.
- `3d/src/src.md`, `3d/tests/tests.md` — entries for `landscape.c` mention painting.

## Done when
Test `3d/landscape` passes with the cases above.
