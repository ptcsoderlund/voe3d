# 08 — A 3d frame carries and draws the world's fog
folder: 3d
after: 05, 07
decisions: 0168, 0480, 0493

## Change
0493 points 3 and 5 on 3d's side: the frame holds the fog from its eye, hands it to the pass
camera, and the run draws it right after the clouds. Read in `3d/include/3d/draw_system.h` the
`voe_3d_frame` struct's `clouds` member and the comments of `_frame`, `_camera` and `_run`; the
header of `3d/src/draw_system.c` and its clouds lines; `3d/include/3d/fog_volume.h` and
`3d/tests/fog_volumes.c` (card 07); `voe_render_frame_draw_fog` in
`render/include/render/device.h` (card 05).

- `3d/include/3d/draw_system.h`: `voe_3d_frame` gains `voe_render_fog fog` after `clouds`,
  commented as `clouds` is (filled by _frame from `voe_3d_fog_volumes` at its eye and lag, handed
  to the pass camera). `_camera`'s comment names the fog; `_run`'s says it draws it right after
  the clouds.
- `3d/src/draw_system.c`: `_frame` fills `frame.fog` from `frame.eye` and `frame.lag` after
  `frame.clouds`; `_camera` hands it to the pass camera; `_run` calls `voe_render_frame_draw_fog`
  once, right after `voe_render_frame_draw_clouds`, with no span of its own open; false is a
  stderr line and the run goes on. The file's header names the fog. Keep the file under 800
  lines.
- `3d/tests/fog_volumes.c`: a case: `voe_3d_draw_system_frame` carries the same record as
  `voe_3d_fog_volumes` with its eye and lag, and `voe_3d_draw_system_camera` hands it on.
- `3d/3d.md`, `3d/include/3d/3d.md`, `3d/src/src.md`: the draw_system entries name the Fog
  volume rows where they name the Clouds row, each at most 300 characters.

## Done when
The tests `3d/fog_volumes`, `3d/clouds`, `3d/draw_system` and `3d/sky` pass after the folder's
build.
