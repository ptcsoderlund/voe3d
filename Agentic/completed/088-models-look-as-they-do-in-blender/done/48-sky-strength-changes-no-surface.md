# 48 — Sky strength changes no surface
folder: render
after: none
decisions: 0168, 0411, 0423, 0424

## Change
0424 point 4: the sky a surface mirrors is the backdrop at strength 1, so the Sky row's strength only
scales the drawn backdrop and the haze.

- `render/shaders/sky.slangh`: add `float3 voe_render_sky_radiance_of(voe_render_sky sky, float3 view)`,
  the radiance of the sky record it is handed (backdrop by `voe_render_sky_gradient` of that record,
  then the frame's shown lights' glows and discs as now); `voe_render_sky_radiance(view)` becomes that
  of the frame block's sky. Same arithmetic, so the drawn sky is unchanged. Header: the new call, and
  that it brings the shader's signature nearer the CPU twin's.
- `render/shaders/lighting.slangh`: `voe_render_sky_reflection` mirrors
  `voe_render_sky_radiance_of` of a copy of the frame's sky with `strength` 1. The header's "THE SKY
  LIGHTS NOTHING" paragraph: the mirrored sky and its harmonics are at strength 1 whatever the row's
  strength (0423 point 3, 0424 point 4).
- `render/src/pass_sky.c`: `voe_render_pass_place_sky_light` projects the harmonics of a copy of
  `camera->sky` with `strength` 1. Its function comment and the file header say so.
- `render/src/device_parts.h`: the `sky_harmonics` paragraph says they are of the backdrop at strength 1.
- `render/src/src.md`: the `pass_sky.c` entry, if it fits.
- `render/tests/sky_light.c`: a new check: the smooth white metal quad of check 3, under the same
  noon sun, reads the same bytes with the sky at strength 0 and at strength 1, and so does the grey
  cube's face of check 1. Add it to the numbered list in the header.
- `render/tests/tests.md`: the `sky_light.c` entry names it, at most 300 characters.

## Done when
The test `render/sky_light` passes after the folder's build, with the new strength check in it.
