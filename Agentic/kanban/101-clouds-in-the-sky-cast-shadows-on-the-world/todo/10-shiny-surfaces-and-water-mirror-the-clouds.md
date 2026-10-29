# 10 — Shiny surfaces and water mirror the clouds
folder: render
after: 09
decisions: 0168, 0418, 0423, 0492

## Change
0492 point 8: where a surface mirrors the sky it mirrors the clouds over it. Read the reflection
part and header of `render/shaders/lighting.slangh`, the header and reflection of
`render/shaders/water.slangh`, `voe_render_clouds_reflected` in `render/shaders/clouds.slangh`, the
object record's `sky` comment and `voe_render_sky`'s reflection paragraph in
`render/include/render/device.h`, and `render/tests/sky_light.c` and `render/tests/water.c` as
test patterns.

- `render/shaders/lighting.slangh`: a metal's sharp sky reflection (0412 point 5) lays
  `voe_render_clouds_reflected` along the reflected direction over the sky it reads, before
  0423's brightness by the light on the surface; the rough backdrop irradiance is unchanged;
  never veiled (0418 point 5). Kind none is the old picture byte for byte.
- `render/shaders/water.slangh`: the colour its fresnel mixes towards is the record's `sky` with
  the clouds' reflection laid over it along the reflected view.
- `render/include/render/device.h`: the reflection paragraph of `voe_render_sky` and the water
  record's `sky` comment each say the clouds show in it.
- `render/tests/cloud_reflections.c` (new), headless, a drawn sky, one light, Flat clouds of
  coverage 1 at 500 m: a smooth metal floor seen at a slant changes by more than 8/255 against
  no clouds, and with coverage 0 is within 1/255; a water plane seen at a slant changes too.
- `render/shaders/shaders.md` (`lighting.slangh`, `water.slangh`), `render/tests/tests.md`: follow,
  each at most 300 characters.

## Done when
The tests `render/cloud_reflections`, `render/sky_light` and `render/water` pass after the
folder's build.
