# 11 — A 3d frame carries and draws the world's clouds
folder: 3d
after: 01, 10
decisions: 0168, 0480, 0492

## Change
0492 points 3 and 5 on 3d's side: the Clouds row and its drift into the pass's record, and the
run drawing them. Read in `3d/include/3d/draw_system.h` the `voe_3d_frame` struct, the comments
of `voe_3d_draw_system_sky`, `_camera` and `_run`; the headers of `3d/src/draw_system.c` and
`3d/tests/sky.c` (the pattern, no graphics card); `scene/include/scene/clouds_component.h`
(card 01); `voe_render_clouds` and `voe_render_frame_draw_clouds` in
`render/include/render/device.h` (cards 02 and 08).

- `3d/include/3d/draw_system.h`: `voe_3d_frame` gains `voe_render_clouds clouds` after `exposure`,
  commented as `sky` is; `voe_render_clouds voe_3d_draw_system_clouds(const voe_ecs_world *world,
  voe_math_double3 eye)` declared with a comment: the first Clouds row whose entity is on (0440),
  zeroed with no table, no row or an off one; kind Flat or Volumetric as render's; coverage,
  thickness and shadow strength as authored; `bottom` its height − `eye.y` in double; `origin`
  `eye.x` − drift x and `eye.z` − drift z wrapped into 0 to `VOE_RENDER_CLOUDS_PERIOD` in double,
  no drift row read as 0; public so a view with its own eye carries the same clouds, as `_sky`.
  `_camera`'s comment names the clouds; `_run`'s says it draws them after the world's blended
  draws and before the overlay's depth clear.
- `3d/src/draw_clouds.c` (new): `voe_3d_draw_system_clouds`, with its header.
- `3d/src/draw_system.c`: `_frame` fills `frame.clouds` from `frame.eye`; `_camera` hands it to the
  pass camera; `_run` calls `voe_render_frame_draw_clouds` once, after the world layer's blended
  draws and before the overlay's first depth clear, with no span of its own open; false is a
  stderr line and the run goes on. The file's header names the clouds.
- `3d/tests/clouds.c` (new), no graphics card: no table and no row are zeroed; a Flat row maps to
  `VOE_RENDER_CLOUDS_FLAT` with its fields, a Volumetric one to `_VOLUMETRIC`; an eye at y 200
  under a height of 1500 gives `bottom` 1300; an eye at x 8200 with no drift gives origin x 8;
  a drift of x 10 at eye x 5 gives origin x 8187; an off row is zeroed and the second row on is
  used; `voe_3d_draw_system_frame` carries the same record as the call with its eye.
- `3d/3d.md`, `3d/include/3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md`: the draw_system entries
  name the Clouds row where they name the Haze's; new entries for the new files; each at most
  300 characters.

## Done when
The tests `3d/clouds`, `3d/sky`, `3d/draw_system` and `3d/draw_water` pass after the folder's
build.
