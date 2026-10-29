# 09 — Clouds shadow what the lights reach
folder: render
after: 08
decisions: 0168, 0276, 0480, 0492

## Change
0492 point 7: every lit surface's direct light from each directional light is dimmed by the
clouds above it along that light, by the row's shadow strength; the fill, the objects' shadows
and the bounce are not. Read the header of `render/shaders/lighting.slangh` and its directional
lights' part, the header and lighting of `render/shaders/water.slangh`,
`voe_render_clouds_transmittance` in `render/shaders/clouds.slangh`, the `voe_render_light`
comment in `render/include/render/device.h`, and `render/tests/directional_lights.c` and
`render/tests/water.c` as test patterns.

- `render/shaders/lighting.slangh`: for the first light and each of `more`, the direct term (the
  part `saturate(N·L) × shadow` scales) is multiplied by `1 − shadow_strength × (1 − T)`, T
  `voe_render_clouds_transmittance` at the surface towards that light. The reach that weighs the
  fill (0276) is taken before it and does not change. Header: a paragraph saying so, and that
  kind none is the old picture byte for byte.
- `render/shaders/water.slangh`: its directional light 0 dimmed the same way; header follows.
- `render/include/render/device.h`: `voe_render_clouds`'s comment says what its shadow does and
  does not touch (0492 point 7), in a few lines.
- `render/tests/cloud_shadows.c` (new), headless, a floor seen from above, one light from above
  with a fill, clouds Volumetric coverage 1, bottom 500, thickness 500: shadow strength 1 darkens
  the floor by more than 8/255 against no clouds; 0 is the same bytes as none; 0.5 lies between;
  a floor facing away from the light, lit by the fill alone, is within 1/255 with and without
  clouds at strength 1; clouds whose bottom is 200 m below the floor change nothing; a water
  plane under strength 1 is darker than without.
- `render/shaders/shaders.md` (`lighting.slangh`, `water.slangh`), `render/tests/tests.md`: follow,
  each at most 300 characters.

## Done when
The tests `render/cloud_shadows`, `render/directional_lights`, `render/water` and
`render/shadow_lights` pass after the folder's build.
