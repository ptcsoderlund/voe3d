# render

The GPU, and the only folder that names Vulkan. A device opened onto a window, a
swapchain, two geometry pools and a transient pair per frame slot, textures,
shading records, six pipelines, the sun's and point lights' shadow maps, a frame and a GPU
texture encoder — nothing above them.

- `include` — the public header, in `include/render/`. See
  `include/render/render.md`.
- `src/` — the implementation behind that header: the loader and the two window
  backends, startup, the pools, the six pipelines, the sky drawn first in a camera pass, the shadow maps,
  the frame and the texture encoder. See
  `src/src.md`.
- `tests/` — sixteen programs, most of them headless, that check what reached the
  picture rather than what the bookkeeping said. See `tests/tests.md`.
- `shaders/` — the Slang shaders and their parts, compiled and embedded at build time,
  every mesh exit exposed and tone mapped. See
  `shaders/shaders.md`.
- `vulkan/` — the Khronos headers, vendored. See `vulkan/vulkan.md`.
