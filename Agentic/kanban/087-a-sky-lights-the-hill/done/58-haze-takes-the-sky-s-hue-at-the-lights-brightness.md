# 58 — Haze takes the sky's hue at the lights' brightness
folder: render
after: none
decisions: 0168, 0416, 0417

## Change
Bug 08: under a drawn sky the haze is the lights' colour (0415); it should be the sky's horizon hue
at the lights' brightness (0416 points 2 to 4, 0417 point 2). The record does not change. Read the
headers of `render/shaders/tone.slangh`, `render/shaders/sky.slangh`, `render/src/sky.h` and
`render/tests/haze.c`, and the `voe_render_sky` comment in `render/include/render/device.h`.

- `render/shaders/tone.slangh`: `voe_render_haze_colour` sums, over the same shown lights, each
  light's largest colour channel × intensity × `VOE_RENDER_HAZE_GAIN` into `B`. With
  `voe_render_frame.sky.drawn` it returns `hue × B`, `hue` from `voe_render_sky_gradient` on a copy
  of the sky at strength 1 along a level view (`view.y` 0), divided by its largest channel, white
  when that is not above 0. Without `drawn` it returns 0415's sum as now. Its comment and the
  header's haze sentence say so (0416), and that sky strength and colour brightness never reach it.
- `render/include/render/device.h`, `voe_render_sky` comment: the haze paragraph says `haze` and
  `haze_start` are the scene's Haze row's and haze whether or not `drawn`; with `drawn` the colour
  is the horizon hue × the lights' brightness, without it the lights' colour; no shown light,
  towards black. `strength` keeps "not the haze". Struct and asserts unchanged.
- `render/tests/haze.c`: computes `hue` with `voe_render_sky_gradient` from `../src/sky.h` as the
  shader does, and `B` from the lights. Cases with the sky drawn now expect `hue × B` where they
  expected `H`:
  1, 2, 5, 9 take `hue × B`; 6 unchanged;
  7. strength 0.25 and zenith, horizon and ground each × 0.1: within 3/255 of case 1;
  10. the orange light (1, 0.5, 0.2) at intensity 3: (1 − 2^−2) × hue × 3/(4π), its colour not in it;
  11. the sun plus the `more` light: (1 − 2^−2) × hue × the sum of both lights' `B`;
  12 unchanged; new cases, each at 2000 m:
  13. horizon (0.9, 0.5, 0.2): (1 − 2^−2) × that hue × `B`;
  14. horizon (0, 0, 0): (1 − 2^−2) × white × `B`;
  15. sky not drawn, the orange light of case 10: (1 − 2^−2) × its colour × 3/(4π) (0415 kept).
  Each within 3/255 of the sRGB of the value named. The header's list follows.
- `render/shaders/shaders.md` (`tone.slangh`), `render/tests/tests.md` (`haze.c`): changed where
  what they say changed, each one sentence of at most 300 characters.

## Done when
The tests `render/haze`, `render/sky`, `render/sky_model`, `render/exposure` and `render/water` pass
after the folder's build.
