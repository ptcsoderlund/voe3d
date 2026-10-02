# 14 — A pass bends each point light's reach by its falloff
folder: render
after: none
decisions: 0168, 0321, 0322

## Change
The record a pass carries holds the light's falloff and draw.slang's fade uses it (0322 points 1–2).
Read the point light section of `render/include/render/device.h`, the headers of
`render/src/pass.c` and `render/shaders/lighting.slangh`, the point light struct in
`render/shaders/bindings.slangh`, and `render/tests/point_lights.c`.

- `render/include/render/device.h`: `voe_render_point_light`'s `reserved` becomes `float falloff`,
  same place, still 32 bytes. Comment points: the curve `saturate(1 − (d/range)^(2/falloff))²`,
  falloff 1 being the old `(1 − (d/range)²)²`; falloff as the scene authored it; finite and above
  nought or the pass asserts.
- `render/src/pass.c`: at pass begin, where the lights are copied, assert each light's falloff is
  finite and above nought. Header point on it.
- `render/shaders/bindings.slangh`: the struct's `reserved` becomes `falloff`, matching the C.
- `render/shaders/lighting.slangh`: `voe_render_points`' fade becomes the curve above; the
  header's point light paragraph and the function's comment give the new curve.
- `render/tests/point_lights.c`: every light it builds sets falloff 1, so the existing cases keep
  their numbers; a new case draws the same light at falloff 0.25, 1 and 4 and reads one pixel at
  about 0.8 of the reach along the ground: brighter at 0.25 than at 1, and at 1 than at 4; and the
  pixel under the light is no darker at 0.25 than at 1.
- `render/include/render/render.md`, `render/shaders/shaders.md`, `render/tests/tests.md`: the
  entries for device.h, lighting.slangh and point_lights.c name the falloff where they name the
  fade. Each at most 300 characters.

## Done when
The test `render/point_lights` passes, and `render/light_bins` and `render/unshaded` still pass,
after the folder's build.
