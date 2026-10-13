# 66 — A 3d frame carries the haze's top above the eye
folder: 3d
after: 63, 65
decisions: 0168, 0418, 0419

## Change
0419 points 1 and 2: the pass needs the haze layer's top in metres above the eye, so
`voe_3d_draw_system_sky` takes the eye. Its callers in `editor` and `game/tests` follow in cards
67 and 68; do not touch them. Read the comment above `voe_3d_draw_system_sky` in
`3d/include/3d/draw_system.h`, the header of `3d/src/draw_system.c`, the `haze_top` paragraph of
`voe_render_sky` in `render/include/render/device.h` (card 63),
`scene/include/scene/haze_component.h` (card 65) and the header of `3d/tests/sky.c`.

- `3d/include/3d/draw_system.h`: `voe_render_sky voe_3d_draw_system_sky(const voe_ecs_world
  *world, voe_math_double3 eye)`, `eye` the camera's world position the frame is about. Its
  comment: from the Haze row also `haze_top`, its `height` − `eye.y` in double, 0 with no Haze
  row; a view building its own frame passes its own eye.
- `3d/src/draw_system.c`: `voe_3d_draw_system_sky` fills `haze_top` so; `voe_3d_draw_system_frame`
  passes the `eye` it has just set, so set `eye` before the sky.
- `3d/tests/sky.c`: every call passes an eye. New record cases: a Haze row of height 300 and an
  eye at y 40, `haze_top` 260; the default Haze row and the origin, 1000; no Haze row, 0; and
  through `_frame`, a camera at y 40 under that row of height 300 frames `haze_top` 260. The
  header's list follows.
- `3d/3d.md`, `3d/include/3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md`: the draw_system and
  sky entries say the sky takes the eye where they name its fields, each one sentence of at most
  300 characters.

## Done when
The tests `3d/sky`, `3d/no_light` and `3d/draw_system` pass after the folder's build.
