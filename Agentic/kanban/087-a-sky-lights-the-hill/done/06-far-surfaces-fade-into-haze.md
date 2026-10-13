# 06 — Far surfaces fade into haze
folder: render
after: 05
decisions: 0168, 0405

## Change
0405 point 6. Read the headers of the files below, `render/shaders/sky.slangh` and
`render/tests/exposure.c` (card 02) as the pattern.

- `render/shaders/tone.slangh`: `float3 voe_render_haze(float3 colour, float3 position)`, `position`
  eye-relative as the draws place it: colour × (1 − f) + sky(v) × f, f = 1 − 2^(−|position|/haze), v
  the view direction with its height clamped to the horizon, sky(v) by `voe_render_sky_radiance` from
  the frame block's light; haze nought, or `plain`, returns `colour`. Header says haze comes before
  exposure and tone. It now includes `sky.slangh`.
- `render/shaders/draw.slang`: the lit, unlit and blended exits haze before `voe_render_expose`, a
  blended one its straight colour with alpha kept; `render/shaders/water.slangh`: its exit likewise.
- `render/include/render/device.h`: the `voe_render_sky` comment says what `haze` does and that the
  haze's colour is the sky's at the horizon whether or not `drawn`.
- `render/tests/haze.c` (new, headless): unlit black quads 50 m and 2000 m ahead, sun 30° up, haze
  1000 m: the far one within 3/255 of the sRGB of 1 − 2^−2 of the horizon's radiance (from
  `../src/sky.h`), the near one darker than 1/10 of it; haze 0 reads black byte for byte; a quad
  drawn after a depth clear reads black; a blended quad at 2000 m keeps its alpha's blend over the
  sky.
- `render/shaders/shaders.md`, `render/tests/tests.md`: entries changed where what they say changed,
  one new for the test. Each at most 300 characters.

## Done when
The tests `render/haze`, `render/exposure`, `render/water` and `render/sky_light` pass after the
folder's build.
