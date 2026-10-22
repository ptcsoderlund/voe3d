# 04 — A landscape's nodes outside a view are skipped
folder: 3d
after: 03
decisions: 0168, 0396, 0397, 0464

## Change
0464 point 2 for the ground: the eye still chooses the same nodes for every pass (0396), and each
pass then skips the chosen nodes its volume cannot see.

- `3d/src/draw_terrain.h`, `3d/src/draw_terrain.c` —
  - `voe_3d_draw_terrain_solid` and `voe_3d_draw_terrain_cast` gain
    `const struct voe_3d_cull *cull` (NULL tests nothing) and `voe_3d_view_counts *counts`
    (NULL counts nothing): a node whose box (its corner and side in the landscape's metres, the
    morph's bottom and height, under the row's world matrix about the eye, as
    `voe_3d_draw_terrain_nodes` records it) has a sphere the volume does not see is not drawn.
    Solid counts into `drawn`/`skipped`, cast into `cast`/`cast_skipped`; a node is one thing.
  - `voe_3d_draw_terrain_hold` (a fading landscape) gains the cull and counts the same way.
  - The header's THE SAME EYE CHOOSES THE SAME NODES paragraph says selection stays the eye's and
    the cull comes after it, so a node never changes level by being culled.
- `3d/src/draw_system.c` — the landscape walk hands `_run`'s cull and counts.
- `3d/src/draw_shadows.c`, `3d/src/draw_bounce.c` — every `voe_3d_draw_terrain_cast` call passes
  NULL, NULL for now; card 05 hands the cascades' cull.
- `3d/tests/draw_terrain.c` — cases: a camera above a landscape looking away from it counts every
  chosen node skipped and none drawn; looking at it, the pixel over the ground is unchanged from
  before and `drawn` is above nought; turned so half the ground is in view, both are above nought.
- `3d/src/src.md`, `3d/tests/tests.md` — entries follow.

## Done when
Tests `3d/draw_terrain`, `3d/terrain_shadow` and `3d/view_culling` pass.
