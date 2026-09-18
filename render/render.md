# render

The GPU, and the only folder that names Vulkan. A device opened onto a window, a
swapchain, two geometry pools and a transient pair per frame slot, textures,
shading records, three pipelines and a frame — nothing above them. Not scenes, not entities, not files, and no abstraction
over Vulkan: there is one graphics API and there will not be a second. The only
folder with shaders, and the only one slangc is run over.

Two of the three pipelines draw meshes out of the pools, one per thing drawn. The
third draws element records — many rectangles from many small records, in one
instanced draw, with no vertex buffer bound at all. A rectangle and a letter are
the same record and the same draw command on that path: an element that reads its
coverage out of a distance-field sheet is a glyph, and nothing about it is a text
system.

A frame is drawn into offscreen images the engine owns — colour and depth, one
pair per frame slot — and the colour one is copied onto the window afterwards.
Nothing draws into a swapchain image, and everything inside one is linear light:
the sRGB curve is a texture format on the way in and the target's format on the
way out, and no file here holds a gamma constant.

- `include` — the public header, in `include/render/`. See
  `include/render/render.md`.
- `src/` — the implementation behind that header: the loader and the two window
  backends, startup, the pools, the three pipelines and the frame. See
  `src/src.md`.
- `tests/` — eight programs, most of them headless, that check what reached the
  picture rather than what the bookkeeping said. See `tests/tests.md`.
- `shaders/` — the three Slang shaders, compiled and embedded at build time. See
  `shaders/shaders.md`.
- `vulkan/` — the Khronos headers, vendored. See `vulkan/vulkan.md`.
