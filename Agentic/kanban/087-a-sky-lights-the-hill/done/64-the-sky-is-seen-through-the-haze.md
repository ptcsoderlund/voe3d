# 64 — The sky is seen through the haze
folder: render
after: 63
decisions: 0168, 0418, 0419

## Change
Bug 09: dense haze greys the ground but the sky and its discs stay clear. 0418 points 2 to 5,
0419 points 3 and 4. Read the headers of `render/shaders/tone.slangh`, `render/src/sky_draw.c`,
`render/tests/sky.c` and `render/tests/haze.c`; the sky stage's comments and functions at the end
of `render/shaders/draw.slang`; the `CLEAR_*` defines in `render/src/pass.c`; the `voe_render_sky`
comment and the "sky drawn first" paragraph above `voe_render_pass_begin` in
`render/include/render/device.h`.

- `render/shaders/tone.slangh`: `VOE_RENDER_CLEAR` (float3), pass.c's clear colour, which it
  must equal. New `float voe_render_haze_veil(float3 view)`: 0419 point 3's `f` from the frame
  sky's `haze`, `haze_start` and `haze_top` along a unit world view. The header and the
  function's comment: the sky's fragment is veiled by it, never a mesh exit, which hazes by
  distance only as before.
- `render/shaders/draw.slang`, `voe_render_sky_fragment`: with `drawn`, expose and tone
  `lerp(radiance, voe_render_haze_colour(), f)`; without, `lerp(VOE_RENDER_CLEAR,
  voe_render_expose(voe_render_haze_colour()), f)` (0419 point 4). The comment above it says so.
- `render/src/sky_draw.c`: records the stage when the block's sky is `drawn` or its `haze` is
  above 0; nothing else changes. Its header follows (what it reads back, when it draws).
- `render/src/pass.c`: the comment on the `CLEAR_*` defines names tone.slangh's twin.
- `render/include/render/device.h`, comments only: the sky stage runs with `drawn` or with haze;
  the sky, its discs and glows, or without `drawn` the clear backdrop, fade towards the haze
  colour along the path through the layer below `haze_top` (0418 point 2); the sky's reflection
  is never veiled (point 5); `haze` 0 is still the old picture.
- `render/tests/haze.c` case 5: set `haze_top` 1000 so the sky behind the centre is fully veiled;
  expect 0.5 × (1 − 2^−2) × hue × B + 0.5 × hue × B. The header follows.
- `render/tests/haze_sky.c` (new), set up as `render/tests/sky.c` (camera at the origin along −Z,
  a white sun 30° up ahead, 0406 point 1's colours drawn, exposure 0, Clip), each pixel read with
  its direction as sky.c reads it; `H` as haze.c computes it (hue × B drawn, the lights' colour
  not drawn); each within 3/255 of the sRGB named:
  1. haze 10, start 0, top 1000: the centre and a pixel near the top, `H`;
  2. haze 2000, start 0, top 1000: the pixel near the top, `lerp(radiance, H, f)` with
     `f = 1 − 2^(−(1000 / v.y) / 2000)`, radiance from `../src/sky.h`; a pixel below the centre, `H`;
  3. haze 10, top −10: the pixel near the top byte for byte as with haze 0;
  4. haze 10, top 1000: the pixel nearest the sun, `H`, not the disc;
  5. not drawn, haze 10, top 1000: the centre, `H`; with top −10, the pixel near the top is the
     clear colour as sky.c's case 3 reads it, byte for byte.
  A machine without Vulkan skips and says so. Header lists the cases.
- `render/shaders/shaders.md` (`tone.slangh`, `draw.slang`), `render/src/src.md`
  (`sky_draw.c`), `render/tests/tests.md` (`haze.c`, new `haze_sky.c`): changed where what they
  say changed, each one sentence of at most 300 characters.

## Done when
The tests `render/haze_sky`, `render/haze`, `render/sky`, `render/sky_light`, `render/exposure`
and `render/water` pass after the folder's build.
