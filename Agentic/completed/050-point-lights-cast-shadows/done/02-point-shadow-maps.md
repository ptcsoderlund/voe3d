# 02 — The point shadow maps: capacity, feature and image
folder: render
after: 01
decisions: 0168, 0325

## Change
0325 point 1. Nothing draws into the maps yet.
- `render/include/render/device.h`: `voe_render_capacities` gains `point_shadow_size` last, said
  in the capacities comment like `shadow_size` (may be nought, cost 6 × 16 × size² × 4 bytes a
  frame slot, nought on a card without `shaderOutputLayer`); `#define VOE_RENDER_POINT_SHADOWS 16`
  beside `VOE_RENDER_SHADOW_CASCADES`; `[[nodiscard]] bool voe_render_point_shadows_ready(const
  voe_render_device *device)`, true when the maps exist.
- `render/src/device.c`: where `VkPhysicalDeviceVulkan12Features` is queried, enable
  `shaderOutputLayer` when `available12` has it; record whether it did; with
  `point_shadow_size` above nought and no feature, one stderr line and the size taken as nought.
  Create the maps after the sun's shadow maps and destroy them beside them.
- `render/src/device_parts.h` / `device_internal.h`: per frame slot the point shadow image, its
  memory, a sampled 2D-array view of all 96 layers and an attachment view of the same; the
  device's size and the feature flag; declarations of the new file's calls.
- `render/src/point_shadow.c`, new, modelled on `render/src/shadow.c` (read its header): create
  and destroy the per-slot D32 2D-array images (one texel when the size is nought, as `shadow.c`
  keeps one for the binding), settle them to shader-read, `voe_render_point_shadows_ready`. Header:
  what the maps are, the layer of slot s face f, why one texel stays.
- `render/src/descriptors.c`: binding 9, the slot's sampled point-shadow view through `shadow.c`'s
  comparison sampler, written per frame slot; the layout's count 9 → 10.
- `render/shaders/bindings.slangh`: declare binding 9 as a `Sampler2DArrayShadow`-style
  comparison array, the way the cascades' binding is declared; nothing reads it yet.
- `render/src/src.md`, `render/shaders/shaders.md`, `render/render.md`,
  `render/include/render/render.md`: entries for the new file and the binding.
- `render/tests/point_shadows.c`, new, headless: a device with `point_shadow_size` 64 is ready
  and draws two frames of a lit cube; one with 0 is not ready and draws the same. Entry in
  `render/tests/tests.md`.

## Done when
`ctest --test-dir build/debug -R "^render/point_shadows$"` passes.
