# render

The GPU, and the only folder that names Vulkan. A device opened onto a window, a
swapchain, two geometry pools and a transient pair per frame slot, textures,
shading records, four pipelines, the sun's shadow maps and a frame — nothing above them.

- `include` — the public header, in `include/render/`. See
  `include/render/render.md`.
- `src/` — the implementation behind that header: the loader and the two window
  backends, startup, the pools, the four pipelines, the shadow maps and the frame. See
  `src/src.md`.
- `tests/` — eleven programs, most of them headless, that check what reached the
  picture rather than what the bookkeeping said. See `tests/tests.md`.
- `shaders/` — the three Slang shaders, compiled and embedded at build time. See
  `shaders/shaders.md`.
- `vulkan/` — the Khronos headers, vendored. See `vulkan/vulkan.md`.
