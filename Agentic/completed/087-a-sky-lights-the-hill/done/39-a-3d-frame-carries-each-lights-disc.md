# 39 — A 3d frame carries each light's disc
folder: 3d
after: 34, 37
decisions: 0168, 0411, 0412

## Change
0412 point 6 in `3d`: the frame's sky takes the sky row's backdrop and each light row's disc and
glow. Read the headers of `3d/include/3d/draw_system.h` (the comment above
`voe_3d_draw_system_sky` and above `voe_3d_draw_system_lights`) and `3d/tests/sky.c`, the
`voe_render_sky` comment in `render/include/render/device.h`, and `voe_3d_draw_system_sky` in
`3d/src/draw_system.c`.

- `3d/src/draw_system.c`, `voe_3d_draw_system_sky`: from the first sky row, `drawn` 1, its
  colours, horizon height, strength and haze; `disc_size[i]` and `glow_size[i]` from light rows 0
  to VOE_RENDER_DIRECTIONAL_LIGHTS − 1 in table order (the order `_light` and `_lights` hand them
  to the pass), 0 past the last row or with no light table. No `fill`, no sun shape from the sky.
  Zeroed as now with no sky table or row.
- `3d/include/3d/draw_system.h`: that comment says so.
- `3d/tests/sky.c`: the records case takes a sky row of non-default values incl. strength 0.5 and
  two light rows with discs 2 and 0.5 and glows 0.4 and 0: `drawn` 1, each sky field the row's,
  `disc_size` {2, 0.5, 0, 0} and `glow_size` {0.4, 0, 0, 0}; with the light table empty both arrays
  nought. The picture case stands; its header lines follow.
- `3d/src/src.md` (`draw_system.c`), `3d/include/3d/3d.md` (`draw_system.h`) and
  `3d/tests/tests.md` (`sky.c`): changed where what they say changed, each one sentence of at most
  300 characters.

## Done when
The tests `3d/sky` and `3d/directional_lights` pass after the folder's build.
