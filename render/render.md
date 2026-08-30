# render

The GPU, and the only folder that names Vulkan. A device opened onto a window, a
swapchain, and a frame — nothing above them. Not scenes, not entities, not files,
and no abstraction over Vulkan: there is one graphics API and there will not be a
second.

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
- `src/device_internal.h` — the struct the three files below share, and where the
  split between them runs.
- `src/device.c` — starting up: the instance, the surface, the graphics card, the
  logical device. Everything that happens once.
- `src/swapchain.c` — the images the window is made of, thrown away and built
  again on every resize.
- `src/frame.c` — one frame: wait, acquire, clear, present.
- `tests/loader.c` — that a machine with a driver and no SDK reaches Vulkan.
- `vulkan/` — the Khronos headers, vendored. See `vulkan/vulkan.md`.
