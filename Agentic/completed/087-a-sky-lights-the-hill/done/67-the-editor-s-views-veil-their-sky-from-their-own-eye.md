# 67 — The editor's views veil their sky from their own eye
folder: editor
after: 66
decisions: 0168, 0419

## Change
Card 66 made `voe_3d_draw_system_sky` take the eye (0419 point 2); the editor's views build
their own frame and call it. Read its comment in `3d/include/3d/draw_system.h` and the header of
`editor/src/view_passes.h`.

- `editor/src/view_passes.c`: where each shown view builds its frame, pass `view->eye`, the eye
  that frame is about. The preview's pass takes its sky from `voe_3d_draw_system_frame` and needs
  nothing.
- `editor/src/view_passes.h`: the header's mention of the world's sky says it is seen from the
  view's own eye, so the haze veils it from where the view stands (0418 point 2).

## Done when
The editor folder builds, and `grep -q "voe_3d_draw_system_sky(world, view->eye)"
editor/src/view_passes.c` exits 0.
