# 01 — Render holds 1024 textures
folder: render
decisions: 0168, 0278

## Change
64 texture slots become 1024 (0278 point 1). Nothing outside `render` changes.

- `render/src/device_internal.h`: `VOE_RENDER_MAX_TEXTURES` 1024.
- `render/shaders/draw.slang` and `render/shaders/elements.slang` (if it declares the array): the
  texture array's length. The comment above `voe_render_textures` in `draw.slang` names the three
  places the number is written; change all three and keep that comment true.
- `render/src/descriptors.c`: whatever sizes the binding and the pool from the constant.
- Startup (read `render/src/startup.h`, then `render/src/card.c` or `render/src/device.c`,
  whichever checks the chosen card's limits): refuse a card whose
  `maxPerStageDescriptorSampledImages`, `maxPerStageDescriptorSamplers`,
  `maxDescriptorSetSampledImages` or `maxDescriptorSetSamplers` is below the constant, returned
  as a failure with one stderr line naming the limit and its value.
- `render/include/render/device.h`: every comment that says sixty-four slots (the capacities'
  `targets` paragraph among them) says 1024.
- New `render/tests/textures.c`: on a headless device (skip without a card, as the other tests
  do), create 1×1 textures until one is refused; at least 1000 succeed and the refusal is
  `VOE_BASE_ERROR_REFUSED`; a quad drawn offscreen with the last texture created as its base
  colour reads that texture's colour. List it in `render/tests/tests.md`.
- `render/src/src.md`, `render/include/render/render.md`: only lines that name 64.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_render $(ninja -C build/debug -t
targets all | grep -oE "^voe_test_render_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R
"^render/"` exits 0, `voe_test_render_textures` among them.
