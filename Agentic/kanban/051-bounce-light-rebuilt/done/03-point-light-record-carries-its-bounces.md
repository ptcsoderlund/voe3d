# 03 — A point light's record carries its bounces and bounce strength
folder: render
after: none
decisions: 0168, 0326

## Change
0326 point 1, the record and its checks only; nothing reads the fields on the GPU yet.
- `render/include/render/device.h`: `voe_render_point_light`'s `reserved[2]` become
  `uint32_t bounces` and `float bounce_strength`; still 48 bytes. `#define VOE_RENDER_BOUNCES_MAX 3`
  beside `VOE_RENDER_POINT_LIGHTS`, the most bounces a light may have (0317). The record's
  comment: what the two mean (how many bounces this lamp's light makes, 0 none; how strong its
  bounce is, scaling the bounce and never the direct light), zero is no bounce so every
  initializer that names neither is as before, and the asserts below.
- `render/src/pass.c`: beside the `shadow_strength` assert in `voe_render_pass_begin`, assert
  `bounces` ≤ `VOE_RENDER_BOUNCES_MAX` and `bounce_strength` finite and not negative.
- `render/shaders/bindings.slangh`: the shader's point light struct names the two words as the
  C one does, so the layouts stay member for member.
- `render/tests/point_lights.c`: a pass with a lamp of bounces 3 and strength 2 draws as the
  same lamp with neither (the fields light nothing directly). Entry in `render/tests/tests.md`.

## Done when
`ctest --test-dir build/debug -R "^render/point_lights$"` passes.
