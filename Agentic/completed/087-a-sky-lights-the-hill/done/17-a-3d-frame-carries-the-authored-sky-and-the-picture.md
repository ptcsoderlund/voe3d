# 17 — A 3d frame carries the authored sky and the picture
folder: 3d
after: 14, 16
decisions: 0168, 0406, 0407

## Change
The new sky row and the picture row into the pass camera (0406 point 4, 0407 point 1). Read the headers
of `3d/include/3d/draw_system.h` (`voe_3d_frame`, `voe_3d_draw_system_sky`, `_frame`, `_camera`,
`_light`) and `3d/src/draw_system.c`, `scene/include/scene/sky_component.h` (card 15),
`scene/include/scene/picture_component.h` (card 16), `voe_render_sky` and `voe_render_exposure` in
`render/include/render/device.h`, and `3d/tests/sky.c`.

- `3d/include/3d/draw_system.h`: `voe_3d_frame` gains `voe_render_exposure exposure` after `sky`,
  commented as `sky` is. `voe_3d_draw_system_sky`'s comment: `drawn` 1, the row's colours, horizon
  height, sun size, sun glow, sunset and haze, and `fill` the first light row's `fill_intensity` (0
  with no light); zeroed with no sky table or row. New
  `voe_render_exposure voe_3d_draw_system_picture(const voe_ecs_world *world)`: the first picture
  row's exposure and tone, zeroed with no picture table or row; public for the same reason as `_sky`.
  The `_frame` and `_camera` comments name the exposure.
- `3d/src/draw_system.c`: `_sky` as above (the light row found as `voe_3d_draw_system_light` finds
  it); `_picture` walks for its table as `_sky` does; `_frame` fills `exposure`; `_camera` passes
  `.exposure`. The static_assert on tone values moves to the picture's constants. Header names the
  picture.
- `3d/tests/sky.c`: no sky table, and a table with no row, zeroed; a row with non-default values
  copied field for field, `fill` the light's fill intensity, and 0 with the light table empty; no
  picture table, zeroed; a picture row of exposure −1, Clip copied and carried by `_camera`. Its GPU
  case: the corner reads `zenith`'s blue above red under the default row; with a picture row of
  exposure −2 the shape's centre is darker than without.
- `3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md`: the draw_system entries name the picture. Each at
  most 300 characters.

## Done when
The tests `3d/sky`, `3d/no_light` and `3d/draw_system` pass after the folder's build.
