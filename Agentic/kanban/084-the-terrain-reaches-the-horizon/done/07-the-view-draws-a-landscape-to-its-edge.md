# 07 — The view draws a landscape's chosen nodes, to its edge
folder: 3d
after: 04, 06
decisions: 0168, 0336, 0388, 0396

## Change
0396 points 3, 4 and 6 in the camera pass. Shadows and the bounce are card 08.

- New internal `3d/src/draw_terrain.h` and `.c` (header comment; entries in `3d/src/src.md`):
  - one call that, for a landscape row's entry, its world place at the frame's lag about
    `frame->eye` and its normal matrix, takes the eye into the grid's space, selects with
    `voe_3d_landscape_select` up to `VOE_3D_LANDSCAPE_NODES` (scratch arena), and fills each node's
    `voe_render_object`: `world` = the row's place × the node box (side × box height × side, box
    height at least 1 mm), `normal` the row's own, `heights`, `terrain`, `morph` as render's header
    says, shading and colour as the part's;
  - one call drawing them solid, and one holding them in a draw group with the part's faded
    record for a fading row, as `model_part_entry` does for a part (0336 point 3).
  Selection depends only on the frame's eye and the row, so card 08's passes get the same nodes.
- `3d/src/draw_system.c`: `draw_models` leaves landscape entries (the internal lookup from card
  06 says which) to draw_terrain; the solid terrain draws of the view pass sit inside
  `voe_render_frame_span_begin(device, "terrain")` … `_end`; `model_part_count` counts
  `VOE_3D_LANDSCAPE_NODES` for a landscape row; a landscape row past `VOE_3D_LANDSCAPES_DRAWN` in
  a frame is not drawn, with one stderr line the first time. The header gains the terrain's
  sentence; `3d/include/3d/draw_system.h`'s `_run` contract says a landscape is drawn by nodes,
  its cost the `terrain` span (0388).
- New headless test `3d/tests/draw_terrain.c` (entry in `3d/tests/tests.md`; build it on the
  device setup `3d/tests/draw_model_fade.c` uses): a 4096 m, 2048-cell landscape, flat but a
  60 m ridge along its far edge, loaded with `voe_3d_models_load_landscape`; a camera 2 m above
  its centre looking level at the ridge, far plane 16000 m. The ridge's pixels above the horizon
  are ground, not sky; a level strip of pixels below the horizon is ground with no sky pixel in
  it; the frame draws at most `VOE_3D_LANDSCAPE_NODES` objects for the row; a row with fade 1
  draws nothing.

## Done when
`3d/tests/draw_terrain.c` passes.
