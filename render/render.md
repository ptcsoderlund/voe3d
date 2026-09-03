# render

The GPU, and the only folder that names Vulkan. A device opened onto a window, a
swapchain, buffers, a pipeline and a frame — nothing above them. Not scenes, not
entities, not files, and no abstraction over Vulkan: there is one graphics API
and there will not be a second. The only folder with shaders, and the only one
slangc is run over.

A frame is drawn into offscreen images the engine owns — colour and depth, one
pair per frame slot — and the colour one is copied onto the window afterwards.
Nothing draws into a swapchain image.

- `include/render/device.h` — the whole public surface: open a device on a
  window, ask it for a frame, destroy it. Its header says why that is one object
  and not four.
- `src/loader.h` — the function-pointer table, and the one place in this engine
  where function pointers are expected. Read its header before adding to it.
- `src/loader.c` — opening the loader by name and filling the table in three
  passes.
- `src/backend.h` — the three things that differ between the two window systems,
  and the whole of what render knows about there being two.
- `src/backend_wayland.c` — the Linux backend. Nothing had to be done about OS
  types here and its header says why.
- `src/backend_win32.c` — the Windows backend, and the seven Windows typedefs
  `vulkan_win32.h` expects `windows.h` to have made. Its header says why each.
- `src/device_internal.h` — the struct the files below share, where the split
  between them runs, and the constants the whole folder reads.
- `src/device.c` — starting up: the instance, the surface, the graphics card, the
  logical device, the pipeline and its depth state. Everything that happens once,
  including the `#embed` that puts the compiled shader in the binary. Its header
  says why the three startup calls are in the order they are. Also the headless
  device the tests run on, and why it is not public.
- `src/buffer.c` — a buffer with the memory under it, and the staging upload that
  fills a device-local one. Its header says why every later upload is this.
- `src/cube.c` — the cube's vertices and indices, the camera and the projection
  matrix, and the descriptor the shader reads them through. Its header says which
  parts of it are placeholders and what replaces them.
- `src/target.c` — the colour and depth images a frame is drawn into, one pair per
  frame slot, thrown away and built again on every resize.
- `src/swapchain.c` — the images the window is made of, thrown away and built
  again on every resize. Nothing draws into them; they are a blit's destination.
- `src/frame.c` — one frame: wait, write the matrices, draw into the target,
  acquire, blit, present. Holds the engine's only Y flip; its header says why
  that is the viewport's job.
- `src/probe.c` — the pipeline that reads a matrix and reports what it saw, built
  only when a test asks. Its header says why it is not built at startup.
- `tests/loader.c` — that a machine with a driver and no SDK reaches Vulkan.
- `tests/matrix.c` — that depth really runs backwards, checked on the CPU, and
  that slangc really was given `-matrix-layout-row-major`, checked by making a
  shader report a known matrix back.
- `tests/offscreen.c` — that back faces are culled, and that the Y flip, the
  cube's winding and the front-face constant agree about which way round that is.
  Headless, so it runs under `ctest` with no window anywhere.
- `shaders/cube.slang` — the cube's two entry points, and the only place a matrix
  is applied to a position.
- `shaders/matrix_probe.slang` — reads a matrix and writes three of its elements
  out as colour, so that a test can tell which layout slangc used.
- `vulkan/` — the Khronos headers, vendored. See `vulkan/vulkan.md`.
