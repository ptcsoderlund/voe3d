# 55 — Haze is lit by the directional lights
folder: render
after: none
decisions: 0168, 0415

## Change
Bug 07: the haze mixes towards the sky's backdrop × strength (0412 point 2), so the sky's colours
reach every hazed surface and the lights do not. 0415 points 1 to 4. Read the headers of
`render/shaders/tone.slangh`, `render/shaders/sky.slangh`, `render/src/sky.h` and
`render/tests/haze.c`, and the `voe_render_sky` comment in `render/include/render/device.h`.

- `render/shaders/tone.slangh`: new `static const float VOE_RENDER_HAZE_GAIN`, 1/(4π).
  `voe_render_haze_colour` loses its `position` parameter: it sums colour × intensity ×
  `VOE_RENDER_HAZE_GAIN` over the frame's lights 0 to `more_count` that `voe_render_sky_shown`
  passes, each by `voe_render_sky_light(i)`, as `voe_render_sky_radiance` walks them; none is 0.
  `voe_render_haze` calls it; its fade and haze start do not change. The function's comment and the
  header's haze sentence say the colour is the shown directional lights' (0415), no fill, shadow or
  view term, the sky's colours and strength not read.
- `render/shaders/sky.slangh` and `render/src/sky.h`: the comments calling the backdrop "the haze's
  colour" (the header's backdrop paragraph and the comment above `voe_render_sky_gradient`, in each
  file) drop that claim.
- `render/include/render/device.h`, `voe_render_sky` comment: `strength` scales the backdrop, not
  the haze; the haze paragraph says the colour is 0415 point 1's, and with no shown light far
  surfaces fade towards black. Struct and asserts unchanged.
- `render/tests/haze.c`: expected haze colour `H` = the test's sun colour × intensity / (4π),
  computed in the test; `../src/sky.h` dropped if nothing else uses it. Cases follow, header list
  likewise:
  1, 2, 5, 9 take `H` in place of the gradient;
  6. the sun 0.5° up ahead and 90° round to the side: each within 3/255 of case 1;
  7. strength 0.25 and zenith, horizon and ground all changed: within 3/255 of case 1;
  10. an orange light (1, 0.5, 0.2) at intensity 3: within 3/255 of the sRGB of (1 − 2^−2) × its
      colour × intensity / (4π);
  11. the test's sun plus one `more` light of another colour (the pass's `more` in
      `voe_render_pass_camera`): within 3/255 of (1 − 2^−2) × the sum of both lights' `H`;
  12. the sun at intensity 0: the black quad at 2000 m black byte for byte, and a white unlit quad
      there within 3/255 of the sRGB of 2^−2 × white.
- `render/shaders/shaders.md` (`tone.slangh`), `render/src/src.md` (`sky.h`),
  `render/tests/tests.md` (`haze.c`): changed where what they say changed, each one sentence of at
  most 300 characters.

## Done when
The tests `render/haze`, `render/sky`, `render/sky_model`, `render/exposure` and `render/water` pass
after the folder's build.

Human, in the editor on lumberhill_project with the sky on and haze above 0 (feature step 7): change
the sky's colours and strength, and the far hazed ground does not change; change the directional
light's colour to orange and raise its strength, and the haze turns orange and brightens with it;
ground nearer than the haze start does not change.
