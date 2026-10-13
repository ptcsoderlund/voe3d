# 51 — Haze leaves surfaces nearer than its start alone
folder: render
after: none
decisions: 0168, 0414

## Change
Bug 06: the haze is counted from the eye, so every surface takes some and a near one washes towards
the horizon colour. 0414 points 1 and 3. Read the headers of `render/shaders/tone.slangh`,
`render/src/pass_sky.c` and `render/tests/haze.c`; the `voe_render_sky` comment and struct and the
pass-begin assert paragraph in `render/include/render/device.h`; the sky struct and its comment in
`render/shaders/bindings.slangh`; the sky asserts in `render/src/records_layout.c`.

- `render/include/render/device.h`: `voe_render_sky`'s first reserved word becomes `float
  haze_start`, `reserved[2]` after it, still 96 bytes. The comment's `haze` paragraph: no haze
  nearer than `haze_start` metres, beyond it the 0414 formula, `haze` the half distance past the
  start; `haze_start` at least 0, 0 the fade from the eye. The pass-begin assert paragraph names
  its bound.
- `render/shaders/bindings.slangh`: `reserved0` becomes `float haze_start`; comment follows.
- `render/src/records_layout.c`: the sky offset assert adds `haze_start` at 52; its message names it.
- `render/src/pass_sky.c`: asserts `haze_start` finite and at least 0 beside `haze`, the message
  naming both.
- `render/shaders/tone.slangh`: `voe_render_haze` takes f from the distance past
  `voe_render_frame.sky.haze_start` (0414 point 1); a surface at or nearer the start returns
  `colour` as it came. Its comment and the header's haze sentence say so.
- `render/tests/haze.c`, new cases, added to the header's list, at the test's sky and haze 1000 m:
  8. `haze_start` 100 m, the quad at 50 m: black, byte for byte as with haze 0;
  9. `haze_start` 1000 m, the quad at 2000 m: within 3/255 of the sRGB of (1 − 2^−1) × the
     gradient along the pixel, as case 1 reads it.
  Cases 1 to 7 keep `haze_start` 0 and do not change.
- `render/shaders/shaders.md` (`tone.slangh`, `bindings.slangh` if it names the sky's fields),
  `render/src/src.md` (`pass_sky.c` if it names what is checked), `render/tests/tests.md`
  (`haze.c`): changed where what they say changed, each one sentence of at most 300 characters.

## Done when
The tests `render/haze`, `render/sky`, `render/sky_model` and `render/exposure` pass after the
folder's build.
