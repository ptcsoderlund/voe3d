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
  window, ask it for a frame with what the person did to the camera, upload a
  texture and choose which one is drawn, destroy it. Its header says why that is
  one object and not four, why the camera is moved by a description of input
  rather than by a matrix, and what the two halves of a texture id are for.
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
- `src/cube.c` — the two cubes: twenty-four vertices and their indices, a matrix each,
  the two cameras — the orbit and the flown one — the projection matrix, and the
  descriptor the shader reads the camera through. Its header says why there are
  two cubes, why there are two cameras, why the per-object matrix is a push
  constant, and which parts of it are placeholders.
- `src/target.c` — the colour and depth images a frame is drawn into, one pair per
  frame slot, thrown away and built again on every resize.
- `src/texture.c` — pixels to a sampled image: the staging copy, the layout
  transitions round it, the mipmap chain, and the slot table the ids name. Its
  header says why mipmaps are generated rather than skipped, what happens on a
  card that cannot filter linearly, and why the format is UNORM today and which
  card changes it.
- `src/swapchain.c` — the images the window is made of, thrown away and built
  again on every resize. Nothing draws into them; they are a blit's destination.
- `src/frame.c` — one frame: wait, move the camera, write it, draw each cube into
  the target, acquire, blit, present. Holds the engine's only Y flip and the
  counted clock the scene moves on; its header says why each is where it is.
- `src/probe.c` — the pipeline that reads a matrix and reports what it saw, built
  only when a test asks. Its header says why it is not built at startup.
- `tests/loader.c` — that a machine with a driver and no SDK reaches Vulkan.
- `tests/matrix.c` — that depth really runs backwards and that the camera's
  orbit keeps its distance, looks at what it orbits and never clips a cube, both
  checked on the CPU; and that slangc really was given
  `-matrix-layout-row-major`, checked by making a shader report a known matrix
  back.
- `tests/offscreen.c` — that back faces are culled, that the Y flip, the cube's
  winding and the front-face constant agree about which way round that is, and
  that a texture arrives the right way up. It reads colours as centroids rather
  than at chosen pixels, and its header says why the camera's angle makes that
  the only stable way to ask. Headless, so it runs under `ctest` with no window
  anywhere.
- `shaders/cube.slang` — the cubes' two entry points, the only place a matrix is
  applied to a position, and the texture array the fragment stage indexes with
  the id the CPU handed out.
- `shaders/matrix_probe.slang` — reads a matrix and writes three of its elements
  out as colour, so that a test can tell which layout slangc used.
- `vulkan/` — the Khronos headers, vendored. See `vulkan/vulkan.md`.
