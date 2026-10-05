# 05 — The shadow array grows to hold several lights
folder: render
after: 02
decisions: 0168, 0357, 0258, 0316

## Change
Decision 0357 point 3, on the device side only: the shader still reads layers 0–3, which is
slot 0. Files:

- `render/include/render/device.h`:
  - add `#define VOE_RENDER_DIRECTIONAL_LIGHTS 4` beside `VOE_RENDER_SHADOW_CASCADES`, with a
    comment: the most directional lights drawn, in table order.
  - `voe_render_shadow.reserved0` becomes `uint32_t slot`. Its comment: which light's four layers
    the record reads; nought is the first, as every zeroed record is.
  - `voe_render_shadow_pass_begin`: its `cascade` becomes `layer`, which is `slot × 4 + cascade`.
    Asserting moves from "below CASCADES" to "below 4 × the lights ready".
  - add `[[nodiscard]] uint32_t voe_render_shadow_lights_ready(voe_render_device *device,
    uint32_t wanted)`. It answers how many lights' maps the array holds now. A `wanted` above that
    (capped at 4) asks for growth at the top of the next frame. Nought on a device whose
    shadow_size is nought. Header points: one GPU wait like a resize, never shrinks, a frame that
    wants more draws with what is ready.
- `render/src/device_parts.h`: `struct voe_render_shadow_map.layers` holds
  `VOE_RENDER_SHADOW_CASCADES × VOE_RENDER_DIRECTIONAL_LIGHTS` views, of which the first
  `held × 4` are made. Comment updated.
- `render/src/device_internal.h`: in `struct voe_render_device`, add `shadow_lights` (held, starting
  at 1) and `shadow_lights_wanted`.
- `render/src/device_calls.h`: add the shadow grow call, shadow.c's.
- `render/src/shadow.c`: make each slot's image with `held × 4` layers. Add the grow function: wait
  idle, free and make every slot's image again at the wanted count, and rewrite binding 5's
  descriptors. Add `voe_render_shadow_lights_ready`. Header points updated.
- `render/src/frame.c`: at frame begin, before the slot's recording opens, grow when wanted is above
  held. Do it where `render/src/bounce_volume.c`'s lazy build is called, and read that call's
  header for the pattern.
- `render/src/descriptors.c`: binding 5's write takes the view of all `held × 4` layers. Read its
  header first; change only that write.
- `render/src/pass.c`: the camera's `shadow.slot` must be below held, else it asserts. The shadow
  pass begin asserts its layer range.
- `render/src/src.md`: the `shadow.c` entry says it grows.
- New `render/tests/shadow_lights.c`, modelled on `render/tests/shadow.c`, with its `tests.md`
  entry. Cases:
  - ready(1) is 1;
  - ready(2) is 1 that frame, and 2 the next;
  - a shadow pass on layer 7 opens and ends, and a frame draws with `shadow.slot = 1`;
  - ready(9) is 4 after a frame;
  - ready(0) is 4 and the array has not shrunk.

## Done when
`ctest --test-dir build/debug -R "^render/(shadow_lights|shadow|passes)$"` passes after the
render build.
