# render

The GPU, and the only folder that names Vulkan. A device opened onto a window, a
swapchain, two geometry pools, textures, shading records, a pipeline and a frame
— nothing above them. Not scenes, not entities, not files, and no abstraction
over Vulkan: there is one graphics API and there will not be a second. The only
folder with shaders, and the only one slangc is run over.

A frame is drawn into offscreen images the engine owns — colour and depth, one
pair per frame slot — and the colour one is copied onto the window afterwards.
Nothing draws into a swapchain image.

- `include/render/device.h` — the whole public surface: open a device, upload
  geometry, textures and shading records and get ids back, then begin a frame,
  draw objects into it and end it. Its header says why nothing here knows what a
  scene is, why an id's index half is the number the shader uses, why geometry
  lives in two shared pools, and what the two padded records are padded for.
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
  device the tests run on.
- `src/descriptors.c` — everything the shader reads and the one layout that
  describes it: four bindings, and one set, one camera buffer and one object
  buffer per frame slot. Its header says which of the four changes how often and
  why writing them per frame is safe.
- `src/buffer.c` — a buffer with the memory under it, and the staging upload that
  fills a device-local one at an offset. Its header says why every later upload
  is this.
- `src/geometry.c` — the two pools and the ranges into them. Its header says why
  a mesh is a range and not a buffer, why the indices are stored as the caller
  numbered them, and why nothing is ever freed.
- `src/shading.c` — the record buffer the fragment stage reads by index, and the
  slots that name its rows. Its header says why one buffer serves every frame
  slot and why creating a record waits for the GPU.
- `src/target.c` — the colour and depth images a frame is drawn into, one pair per
  frame slot, thrown away and built again on every resize.
- `src/texture.c` — pixels to a sampled image: the staging copy, the layout
  transitions round it, the mipmap chain, and the slot table the ids name. Its
  header says why mipmaps are generated rather than skipped, what happens on a
  card that cannot filter linearly, and why the format is UNORM today.
- `src/swapchain.c` — the images the window is made of, thrown away and built
  again on every resize. Nothing draws into them; they are a blit's destination.
- `src/frame.c` — one frame in three calls: wait and open a recording, draw an
  object into it, end it and present. Holds the engine's only Y flip; its header
  says why there are three calls and not one, and why a headless device runs all
  but three lines of it.
- `src/probe.c` — the pipeline that reads a matrix and reports what it saw, built
  only when a test asks. Its header says why it is not built at startup.
- `tests/loader.c` — that a machine with a driver and no SDK reaches Vulkan.
- `tests/pools.c` — two meshes and two ranges, a texture id that stops naming
  anything when it is destroyed, and a full pool as a returned failure. Its
  header says why those two cases are the ones worth a test.
- `tests/matrix.c` — that slangc really was given `-matrix-layout-row-major`,
  checked by making a shader report a known matrix back. Its header says which
  two claims card 018 moved out of it and where they went.
- `tests/offscreen.c` — that back faces are culled, that the Y flip, the winding
  and the front-face constant agree about which way round that is, and that a
  texture arrives the right way up. Holds its own cube and its own camera now,
  and drives the same public calls `3d` does. Headless, so it runs under `ctest`
  with no window anywhere.
- `shaders/draw.slang` — the one pipeline's two entry points, the only place a
  matrix is applied to a position, and the three numbers a draw finds everything
  by. Its header says why the only push constant left is an object's number.
- `shaders/matrix_probe.slang` — reads a matrix and writes three of its elements
  out as colour, so that a test can tell which layout slangc used.
- `vulkan/` — the Khronos headers, vendored. See `vulkan/vulkan.md`.
