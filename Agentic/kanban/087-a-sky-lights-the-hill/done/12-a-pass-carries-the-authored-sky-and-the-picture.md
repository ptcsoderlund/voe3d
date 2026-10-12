# 12 — A pass carries the authored sky and the picture
folder: render
after: 11
decisions: 0168, 0406, 0407

## Change
0407 point 1: the records change shape; the sky is still drawn by the old model (card 13 replaces it)
and still lights by the old rule, its strength now read from `fill` (card 14 makes it the fill). Read
the headers of the files below and `render/src/pass_sky.c` (card 11).

- `render/include/render/device.h`: `voe_render_sky` as 0407 point 1, static_assert 64 bytes;
  `voe_render_exposure` new, static_assert 16 bytes; `VOE_RENDER_TONE_*` now describe it;
  `voe_render_pass_camera` gains `voe_render_exposure exposure` after `sky`. Comment points: each
  sky field, its range (colours at least 0, horizon height, glow, sunset 0 to 1, sun size 0 to 10
  degrees, haze and fill at least 0), zeroed is the old picture; the exposure record carries what the
  sky record used to (stops, curve, plain draws, elements never); the `voe_render_light` comments and
  the `voe_render_pass_begin` asserts paragraph name the exposure record instead of the sky's.
- `render/src/device_parts.h`: the block gains the exposure record between `sky` and
  `sky_harmonics`; `render/shaders/bindings.slangh` twins both records and the block.
  `render/src/records_layout.c` / `render/src/descriptors.c`: their offset and size checks follow
  (the exposure record's own offsets replace the sky's exposure and tone checks).
- `render/src/pass_sky.c`: `voe_render_pass_place_sky` copies both records; asserts every number
  finite and each in the range above, exposure within ±16, tone at most SOFT. A pass with no camera
  and every shadow, point-shadow and capture pass leaves both zeroed, as now. The harmonics are filled
  when `drawn` and `fill` above nought (instead of `light`). Header follows.
- `render/shaders/tone.slangh`: exposure and tone read from the block's exposure record.
  `render/shaders/lighting.slangh`: `voe_render_sky_light`'s strength is `sky.fill`, nought unless
  `drawn`. Headers follow.
- `render/tests/exposure.c`, `haze.c`, `sky.c`, `sky_light.c`, `water.c`: their records move to the
  new fields (`light` becomes `fill` with `drawn` 1, exposure and tone into `.exposure`), same claims.
  `exposure.c` gains a case: a zeroed sky with exposure 1 still doubles, so the picture works with no
  sky.
- `render/render.md`, `render/include/render/render.md`, `render/src/src.md`,
  `render/shaders/shaders.md`, `render/tests/tests.md`: entries changed where what they say changed.
  Each at most 300 characters.

## Done when
The tests `render/exposure`, `render/haze`, `render/sky`, `render/sky_light`, `render/water` and
`render/directional_lights` pass after the folder's build.
