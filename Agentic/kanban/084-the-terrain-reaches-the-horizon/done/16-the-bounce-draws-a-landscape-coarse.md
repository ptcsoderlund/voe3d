# 16 — The bounce draws a landscape coarse
folder: 3d
after: none
decisions: 0168, 0396, 0397

## Change
0397 point 1: capture passes and bounce sun maps draw a landscape at most 16 nodes; the view,
the cascades and the point-shadow pass keep the view's nodes. All in `3d/src` and `3d/tests`.

- `3d/src/draw_terrain.h` / `.c`:
  - `VOE_3D_TERRAIN_BOUNCE_NODES 16`, with a comment: the most nodes a
    bounce pass draws of one landscape, and why (0397).
  - `voe_3d_draw_terrain_cast` gains `uint32_t capacity`: the selection's room, from 1 to
    `VOE_3D_LANDSCAPE_NODES` (asserted). Selection is still from the same eye, so a capacity
    below what the eye wants gives coarser nodes, never other ground.
  - Header: the cast's contract names the capacity; the "same eye chooses the same nodes"
    paragraph says the bounce's passes choose fewer from that eye (0397), and a crack between
    nodes two levels apart is accepted there.
- `3d/src/draw_bounce.h`: `voe_3d_draw_casters` gains `uint32_t landscape_nodes`, the capacity
  handed to the cast for each landscape caster; its contract says so.
- `3d/src/draw_shadows.c`: `voe_3d_draw_casters` and the model-caster walk it calls (around line
  138) pass `landscape_nodes` to the cast; its own calls (around lines 299 and 363: cascades and
  the point-shadow pass) pass `VOE_3D_LANDSCAPE_NODES`. Header: one phrase that the bounce's
  passes take the landscape coarse.
- `3d/src/draw_bounce.c`: both calls of `voe_3d_draw_casters` (around lines 444 and 499: a sun
  map and a capture) pass `VOE_3D_TERRAIN_BOUNCE_NODES`. Header: the capture and sun map draw a
  landscape coarse (0397 point 1).
- `3d/src/src.md`: the `draw_terrain.c` and `draw_bounce.c` entries name the coarse bounce
  selection.
- `3d/tests/draw_terrain.c`: a new case on its landscape and eye: inside an open pass, the cast at
  capacity `VOE_3D_TERRAIN_BOUNCE_NODES` draws at least one and at most that many objects, and at
  capacity 4 still draws (counted as the file's existing node-count case counts draws). Its
  header and `3d/tests/tests.md`'s entry gain the case.

## Done when
`3d/tests/draw_terrain.c` passes, and `3d/tests/bounce.c`, `3d/tests/bounce_scene.c` and
`3d/tests/terrain_shadow.c` still pass.
