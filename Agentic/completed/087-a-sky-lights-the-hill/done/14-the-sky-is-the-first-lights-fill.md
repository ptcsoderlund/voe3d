# 14 — The sky is the first light's fill
folder: render
after: 13
decisions: 0168, 0406, 0407

## Change
0406 point 3 by 0407 point 7. Read the headers of `render/shaders/lighting.slangh` and
`render/shaders/draw.slang`, the `voe_render_sky` and `voe_render_light` comments in
`render/include/render/device.h`, and `render/tests/sky_light.c` and `render/tests/blocker_kinds.c`.

- `render/shaders/lighting.slangh`: `voe_render_directionals` leaves light 0's fill out of the summed
  `fill` where the block's sky is `drawn` and the sky's gate passes at the surface's mask (the gate
  `voe_render_sky_light` already applies; one helper both use, e.g. `bool voe_render_sky_reaches(uint
  mask)`); where the gate fails, or `drawn` is 0, light 0's fill is as now, and lights 1 and up are
  untouched. `voe_render_sky_light` is unchanged but for that helper. Header: the sky paragraph
  says the sky term is light 0's fill under a sky, scaled by `fill`, and the fills paragraph names
  the exception.
- `render/include/render/device.h`: the `voe_render_sky` comment says `fill` is the first light's
  fill intensity and that under a drawn sky the sky stands in for light 0's `fill` outside Indoors
  and Room; the `voe_render_light` `fill` paragraph points to it.
- `render/tests/sky_light.c`, new cases on its shaded face, light 0's `fill` a red (0.5, 0, 0):
  - sky drawn, `fill` 1: the face reads the sky's blue over red, not the red fill;
  - sky drawn, `fill` 0: the face reads black;
  - sky drawn, the cube inside an Indoors blocker: the face reads byte for byte what it reads with the
    sky zeroed;
  - sky zeroed: the face reads the red fill, as before.
  Its existing Indoors case ("reads black") becomes the byte-for-byte case above if it contradicts.
- `render/shaders/shaders.md`, `render/tests/tests.md`: the lighting and test entries say so. Each at
  most 300 characters.

## Done when
The tests `render/sky_light`, `render/directional_lights`, `render/blocker_kinds`, `render/unshaded`
and `render/sky` pass after the folder's build.
