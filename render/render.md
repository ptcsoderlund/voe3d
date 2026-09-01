# render

The GPU, and the only folder that names Vulkan. A device opened onto a window, a
swapchain, a pipeline and a frame — nothing above them. Not scenes, not entities,
not files, and no abstraction over Vulkan: there is one graphics API and there
will not be a second. The only folder with shaders, and the only one slangc is
run over.

A frame is drawn into an offscreen colour image the engine owns and copied onto
the window afterwards. Nothing draws into a swapchain image.

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
- `src/device_internal.h` — the struct the four files below share, and where the
  split between them runs.
- `src/device.c` — starting up: the instance, the surface, the graphics card, the
  logical device, the triangle's pipeline. Everything that happens once,
  including the `#embed` that puts the compiled shader in the binary. Also the
  headless device the offscreen test runs on, and why it is not public.
- `src/target.c` — the offscreen colour image a frame is drawn into, one per
  frame slot, thrown away and built again on every resize. Its header says why
  the layer exists and what it is a starting point for.
- `src/swapchain.c` — the images the window is made of, thrown away and built
  again on every resize. Nothing draws into them; they are a blit's destination.
- `src/frame.c` — one frame: wait, draw into the target, acquire, blit, present.
  Holds the engine's only Y flip; its header says why that is the viewport's job.
- `tests/loader.c` — that a machine with a driver and no SDK reaches Vulkan.
- `tests/offscreen.c` — that back faces are culled, and that the Y flip, the
  triangle's winding and the front-face constant agree about which way round
  that is. Headless, so it runs under `ctest` with no window anywhere.
- `shaders/triangle.slang` — three vertices and their colours, with no vertex
  buffer behind them. The first shader in the engine.
- `vulkan/` — the Khronos headers, vendored. See `vulkan/vulkan.md`.
