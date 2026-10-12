# 13 — The sky is an authored gradient and glow
folder: render
after: 12
decisions: 0168, 0406, 0407

## Change
0406 point 2 by 0407 points 2 to 6, in C and Slang together; 0405's scattering model and its constants
go. Read the headers of the files below.

- `render/src/sky.h`: header rewritten for the new model (the curves in phrases, the endpoints of
  0406 point 1, what "on" means, the twin rule kept, the harmonics leave the disc out but keep the
  glow); the old constants go, 0407's numbers become named `#define`s (WARM, the evening and night
  heights, the dimming, the glow's exponents, the disc's gain and edge, the ground's fade, 256
  directions kept). Signatures become:
  - `voe_math_float3 voe_render_sky_sun(voe_render_light light)`: the light's colour when on, else
    nought.
  - `voe_math_float3 voe_render_sky_radiance(const voe_render_sky *sky, voe_math_float3 direction,
    voe_math_float3 sun, voe_math_float3 view)`, `sun` from the call above.
  - `void voe_render_sky_harmonics(const voe_render_sky *sky, voe_math_float3 direction,
    voe_math_float3 sun, voe_math_float4 out[9])`; `voe_render_sky_irradiance` unchanged.
- `render/src/sky.c`: the three, with its header.
- `render/shaders/sky.slangh`: the same two radiance functions in Slang over the block's
  `voe_render_sky`, the same arithmetic; header as its twin.
- Callers: `render/src/pass_sky.c` (harmonics whenever `drawn` and `fill` above nought, the sun by
  `voe_render_sky_sun`, so a dark sun projects the plain gradient); `render/shaders/draw.slang`'s sky
  fragment (the gradient for an unshaded or dark light, no longer black);
  `render/shaders/lighting.slangh`'s `voe_render_sky_light` and `render/shaders/tone.slangh`'s haze.
  Their headers drop "black under an unshaded light"; so does the `voe_render_sky` comment in
  `render/include/render/device.h`.
- `render/tests/sky_model.c`: rewritten to 0407's claims, defaults of 0406 point 1 unless named, sun
  30° up: straight up equals `zenith`, the horizon facing away from the sun equals `horizon`,
  straight down equals `ground`, each within 1e-4; with no sun the same and no direction brighter
  than the brightest authored channel; at sunset 0 a level view 90° round from the sun reads the same
  with the sun 2° up as 60° up; sunset 1 at sun 2° up: the horizon towards the sun has r > g > b and no channel above
  `horizon`'s largest, the zenith darker than at 30°; sun 15° below at sunset 1, the zenith under a
  fifth of `zenith`; horizon height 0, a view 3° up within 10% of `zenith`, at 1 a view 30° up nearer
  `horizon` than `zenith`; the sun's own direction brighter than any authored channel, sun size 0 no
  disc; irradiance within 5% of a brute-force cosine-weighted sum over 4096 directions, as now.
- `render/tests/sky.c`: compares to the new C within 2/255 as now; its "black sky" case becomes a
  sun of intensity 0 reading the plain gradient. `render/tests/haze.c`, `render/tests/sky_light.c`:
  call the new signatures; `sky_light.c`'s evening case is sunset 1, sun 2° up on the face's side,
  the face's red over blue above noon's.
- `render/src/src.md`, `render/shaders/shaders.md`, `render/tests/tests.md`: the sky entries say
  gradient and glow, not scattering. Each at most 300 characters.

## Done when
The tests `render/sky_model`, `render/sky`, `render/haze` and `render/sky_light` pass after the
folder's build.
