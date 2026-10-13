# 33 — The sky lights nothing
folder: render
after: none
decisions: 0168, 0411, 0412

## Change
0411 point 1 and 0412 point 5: lighting goes back to the lights alone; only metal mirrors the sky.
The sky record does not change here (card 34). Read the header of `render/shaders/lighting.slangh`
and the comments above `voe_render_sun_reach`, `voe_render_fill_weight`, `voe_render_fill`,
`voe_render_sky_light` and `voe_render_directionals` in it; the lit path's lines around
`voe_render_directionals` in `render/shaders/draw.slang`; the `voe_render_light` and
`voe_render_sky` comments in `render/include/render/device.h`; `render/tests/sky_light.c`.

- `render/shaders/lighting.slangh`:
  - `VOE_SUN_TERMINATOR_EDGE`, `VOE_SUN_LIFTED_FLOOR` and `voe_render_sun_reach`'s lifted branch go;
    every light's reach is N·L × shadow.
  - `voe_render_directionals` loses `out float sky_weight` and the sky's stand-in for light 0's
    fill: every light's fill is summed as for a skyless frame.
  - `voe_render_sky_light` becomes `voe_render_sky_reflection(float3 n, float3 v, float3 f0, float
    roughness, float metallic, uint mask)`: the reflection half only, times `metallic`; no diffuse,
    no `fill`. Still nought unless `drawn` and where `voe_render_sky_reaches` fails.
  - Header: the directional paragraph drops the lifted reach and the sky's stand-in; the sky
    paragraph says the sky lights nothing and a metal mirrors it (0411, 0412 point 5).
- `render/shaders/draw.slang`: the lit path calls the reflection with the surface's metalness, no
  `sky_weight`; occlusion as now. Nothing else changes.
- `render/include/render/device.h`, comments only: `voe_render_light`'s fill paragraph loses its
  sky sentence; `voe_render_sky`'s `drawn` paragraph loses the lifted reach, and its `fill`
  paragraph says the field is no longer read (it leaves in card 34) and that the sky's reflection
  shows on metal only.
- `render/tests/sky_light.c` rewritten to these claims, the header's list with them; the sky drawn
  at the test's values, light 0's fill red (0.5, 0, 0):
  1. the grey cube's +Z face, the sun 50° up behind it and then 2° up on its side: byte for byte
     the same with the sky drawn (haze 0) and zeroed, at fill intensity 0 and at the red fill;
  2. the same face reads the same bytes whatever the sky's zenith (blue, then red);
  3. the smooth white metal quad: the sRGB of `voe_render_sky_radiance` along R times Karis's fit
     within 6/255, and at roughness 1 nearer E(R)/π than the sky along R, as now;
  4. that quad as a non-metal (metallic 0, white): byte for byte the same with the sky zeroed;
  5. the metal quad inside an Indoors blocker: byte for byte as with the sky zeroed.
  Cases about fill under a sky, sky weight or the lifted sun go.
- `render/shaders/shaders.md` (`lighting.slangh`) and `render/tests/tests.md` (`sky_light.c`): say
  so, each entry one sentence of at most 300 characters (the suite's finding on `sky_light.c`).

## Done when
The tests `render/sky_light`, `render/directional_lights`, `render/blocker_kinds`,
`render/unshaded` and `render/sky` pass after the folder's build.
