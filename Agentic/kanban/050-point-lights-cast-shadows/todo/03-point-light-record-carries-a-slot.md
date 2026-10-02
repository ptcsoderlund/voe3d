# 03 — A point light's record names its shadow slot and strength
folder: render
after: 02
decisions: 0168, 0325

## Change
0325 point 4, the record and its checks only; nothing reads the fields on the GPU yet.
- `render/include/render/device.h`: `voe_render_point_light` gains, after `falloff`,
  `uint32_t shadow` (slot 1..`VOE_RENDER_POINT_SHADOWS`, 0 none), `float shadow_strength` (0 to 1)
  and two reserved words; the `static_assert` says 48 bytes, three float4s. Its comment: drop "it
  casts no shadow", say what the two fields mean, that zero is no shadow so every existing
  initializer draws as before, and the two asserts below. `voe_render_pass_camera`'s comment: drop
  "casting no shadow".
- `render/shaders/bindings.slangh`: the Slang point light struct matches, field for field.
- `render/src/descriptors.c`: the point light buffer's size follows the struct (read how it is
  sized; fix any static_assert naming 32 bytes).
- `render/src/pass.c`: where `voe_render_pass_begin` copies the points, assert a slot within
  0..`VOE_RENDER_POINT_SHADOWS`, no slot named twice in one pass, and a strength within 0..1 and
  finite. The header's summary of what pass.c checks, one phrase.
- `render/tests/point_lights.c`: one more case, a light with `shadow` 3 and strength 1 lights the
  ground exactly as with 0 (no lookup yet). Update its entry in `render/tests/tests.md`.

## Done when
`ctest --test-dir build/debug -R "^render/point_lights$"` passes.
