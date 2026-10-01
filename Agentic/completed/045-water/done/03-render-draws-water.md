# 03 — render draws water
folder: render
after: 02
decisions: 0168, 0305

## Change
0305 points 2–4. Read `render/include/render/device.h` (the shading values,
the object record, `voe_render_frame_draw_blended`),
`render/src/descriptors.c` (the size asserts), `render/shaders/draw.slang`,
`render/shaders/shaders.md`, and `render/tests/shadow.c` for a headless
lit-and-shadowed scene.

- `device.h`:
  - `voe_render_shading_values`: `uint32_t water` takes the first word of
    `reserved_c` (which becomes one word); its comment: non-zero is the
    water path, zero is every other surface as now.
  - `voe_render_object`: two `voe_math_float4` at the end, `waves` (height,
    length, seconds, deep) and `sky` (rgb, w reserved); comment: read only
    by a water record, zero elsewhere, seconds below 60 and why.
- `descriptors.c`: the asserts for both sizes.
- `draw.slang`, the mirrors and the water path in the fragment stage:
  - the plane's normal and two tangents from the object's matrices, the
    plane's metres from model XY and the world columns' lengths;
  - the four waves' height derivatives (0305 point 3) bend the normal;
  - lit through the existing sun, shadow and fill with roughness 0.05;
  - Schlick from 0.02 toward `sky`;
  - thickness from the pass's depth copy (card 02's slot) and the pass
    projection's z and w rows; coverage and darkening over `deep`; no copy
    is deep;
  - out premultiplied.
  New functions each a few lines, named for what they do.
- `render/tests/water.c`, new, headless: a ground quad sloping down under a
  flat water quad drawn blended after a copy, a sun from above; cases:
  - where the ground meets the water the pixel is near the ground's colour;
    where it is deep, near the water's darkened colour;
  - a pixel's colour changes between seconds 0 and 1.3;
  - seconds 0 and 60 are the same picture within one step of 8 bits;
  - a sun at the mirror angle gives a brighter peak than one behind;
  - a shadow caster over the water darkens the pixels under it;
  - with `water` 0 the quad draws as an ordinary blended surface.
- `render/include/render/render.md`, `render/shaders/shaders.md`,
  `render/tests/tests.md`: entries changed or added.

## Done when
The test `render/water` passes, and `render/offscreen`, `render/shadow`
and `render/depth_copy` still pass, after the folder's build.
