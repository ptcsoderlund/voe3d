# 27 — Haze takes the sky's gradient without its sun
folder: render
after: 26
decisions: 0168, 0409

## Change
Bug 03: a low sun's disc (and glow) sits in the haze colour of every ground point in its azimuth,
because `voe_render_haze_colour` clamps the view to the horizon and reads the whole radiance. Read the
headers of `render/src/sky.h`, `render/src/sky.c`, `render/shaders/sky.slangh`,
`render/shaders/tone.slangh` and `render/tests/haze.c`, and the `haze` paragraph of the
`voe_render_sky` comment in `render/include/render/device.h`.

- `render/src/sky.h` / `render/src/sky.c`: new `voe_math_float3 voe_render_sky_gradient(const
  voe_render_sky *sky, voe_math_float3 direction, voe_math_float3 sun, voe_math_float3 view)`: 0407
  points 3 and 4 only (evening shift and dims), no glow, no disc. `voe_render_sky_radiance` becomes
  it plus the glow and disc, its results unchanged. The header names the gradient as the haze's
  colour and says the radiance is it plus the sun's two.
- `render/shaders/sky.slangh`: the twin `float3 voe_render_sky_gradient(voe_render_sky sky, float3
  direction, float3 sun, float3 view)`, same split, same arithmetic; header and usage lines likewise.
- `render/shaders/tone.slangh`: `voe_render_haze_colour` calls `voe_render_sky_gradient`; its comment
  and the header say the haze is the gradient at the horizon, no disc and no glow (0409).
- `render/include/render/device.h`: the `haze` paragraph says the colour is the sky's gradient at the
  horizon, without the sun's disc and glow.
- `render/tests/haze.c`: its expected value and helper (around line 194) take
  `voe_render_sky_gradient`; header's case 1 says so. New case: the sun 0.5° up straight ahead
  (its disc reaching the horizon), sun size and glow at the test's sky values, the far quad at
  2000 m reads within 3/255 of the sRGB of (1 − 2^−2) × the gradient along the pixel, and no
  channel above the reading at the same distance with the sun 90° round to the side plus 3/255.
  Added to the header's list.
- `render/tests/sky_model.c`: one new case: with the sun 30° up, for a few views (up, the horizon
  away, the sun's own direction) `voe_render_sky_gradient` is at most `voe_render_sky_radiance`
  per channel and strictly less along the sun's own direction; with sun size and glow both 0 the
  two are equal within 1e-4 along each.
- `render/src/src.md` (`sky.h`, `sky.c`), `render/shaders/shaders.md` (`sky.slangh`,
  `tone.slangh`), `render/tests/tests.md` (`haze.c`, `sky_model.c`): changed where what they say
  changed. Each at most 300 characters.

## Done when
The tests `render/haze`, `render/sky_model`, `render/sky` and `render/sky_light` pass after the
folder's build.

Human, in the editor on lumberhill_project with haze on: turn the sun low over the horizon and look
towards it; no bright column runs down the ground, and the sun shows only in the sky.
