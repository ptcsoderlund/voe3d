# render

The GPU, and the only folder that names Vulkan. A device opened onto a window, a
swapchain, two geometry pools and a transient pair per frame slot, textures,
shading records, two pipelines and a frame — nothing above them. Not scenes, not entities, not files, and no abstraction
over Vulkan: there is one graphics API and there will not be a second. The only
folder with shaders, and the only one slangc is run over.

A frame is drawn into offscreen images the engine owns — colour and depth, one
pair per frame slot — and the colour one is copied onto the window afterwards.
Nothing draws into a swapchain image, and everything inside one is linear light:
the sRGB curve is a texture format on the way in and the target's format on the
way out, and no file here holds a gamma constant.

- `include/render/device.h` — the whole public surface: open a device, upload
  geometry, textures and shading records and get ids back, then begin a frame
  with a camera and a sun, draw objects into it and end it. Its header says why
  nothing here knows what a scene is, why an id's index half is the number the
  shader uses, why geometry lives in two shared pools, what the padded records
  are padded for, why a colour texture and a data texture are not
  interchangeable, why a texture's kind and its sampling mode are two
  independent questions, why the sampling modes' names now say more than they
  mean, why exactly one of them filters and what a texture is claiming by asking
  for it, and why an object carries a normal matrix as well as a world one. It also carries the two things card 020 added: how long the card spent on
  a frame, which is its own clock and runs two frames behind, and which of the
  two present modes a caller wants — mailbox is optional and falling back to
  fifo is not a failure. Card 021a added the blended draw beside the solid one:
  its header says what the three alpha modes mean, why the depth write goes off
  and the order therefore matters, and that the colour target holds premultiplied
  colour. Card 021b added `unlit` to a shading record, in the four bytes that
  used to be reserved: its header says why not being lit is a property of a
  surface rather than of a pass. Card 024 added the depth clear a caller makes
  mid-frame: its header says why it takes no clear value and what the colour
  attachment does while it happens. Card 022 added the rectangle of the base
  colour texture a record reads: its header says why that is what puts a sheet of
  frames behind one geometry, and why the whole texture is not a zeroed rect.
  Card 028 added the transient create beside the static one: its header says
  which of the two is a startup operation and which lives inside a frame, why
  one id type serves both, and why the caller builds the arrays rather than
  writing into the pool.
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
  logical device, and the two pipelines with their depth and blend state. Everything that happens once,
  including the `#embed` that puts the compiled shader in the binary. Its header
  says why the three startup calls are in the order they are, and why the format
  it asks the surface for is an sRGB one now that a frame is drawn in linear
  light. Also the headless device the tests run on.
- `src/descriptors.c` — everything the shader reads and the one layout that
  describes it: four bindings, and one set, one camera buffer and one object
  buffer per frame slot. Its header says which of the four changes how often and
  why writing them per frame is safe.
- `src/buffer.c` — a buffer with the memory under it, and the staging upload that
  fills a device-local one at an offset. Its header says why every later upload
  is this.
- `src/geometry.c` — the two static pools, the transient pair in every frame
  slot, and the ranges into all of them. Its header says why a mesh is a range
  and not a buffer, why the indices are stored as the caller numbered them, why
  nothing static is ever freed, how the slot table splits into two bands, why
  the top-of-frame reset is the existing staleness check firing on a schedule,
  where the generation can wrap and why that is noted rather than handled, and
  why a barrier appearing in the transient path would be the bug.
- `src/shading.c` — the record buffer the fragment stage reads by index, and the
  slots that name its rows. Its header says why one buffer serves every frame
  slot and why creating a record waits for the GPU.
- `src/target.c` — the colour and depth images a frame is drawn into, one pair per
  frame slot, thrown away and built again on every resize.
- `src/texture.c` — pixels to a sampled image: the staging copy, the layout
  transitions round it, the two samplers, and the slot table the ids name. Its
  header says why there are no mipmaps and no linear filtering anywhere and what
  that costs, why the two sampling modes now differ only in addressing, and why
  there are two formats — one for a picture of a colour and one for a picture of
  numbers.
- `src/swapchain.c` — the images the window is made of, thrown away and built
  again on every resize. Nothing draws into them; they are a blit's destination.
  It is also where a requested present mode becomes the one in force, and where
  the fallback to fifo happens.
- `src/frame.c` — one frame in three calls: wait and open a recording, draw an
  object into it solid or blended, end it and present. Holds the engine's only Y
  flip and the clear colour; its header says why there are three calls and not
  one, why the camera and the sun share one buffer, why the two draw calls differ
  in one argument, and why a headless device runs all but three lines of it. The
  mid-frame depth clear is the one command here recorded between draws that is
  not a draw, and its comment says why it is not a second rendering block. Since
  card 028 a draw may come out of either pool pair, and its header says why the
  pair is rebound only when a range's pool differs from the one last bound, and
  why the transient reset runs after the fence and nowhere else. The two timestamps that measure the card are written and
  read here, and its header says why they can only be read one lap late and why
  a reading has to be masked before it is subtracted.
- `src/probe.c` — the pipeline that reads a matrix and reports what it saw, built
  only when a test asks. Its header says why it is not built at startup.
- `tests/loader.c` — that a machine with a driver and no SDK reaches Vulkan.
- `tests/pools.c` — two meshes and two ranges, a texture id that stops naming
  anything when it is destroyed, and a full pool as a returned failure. Its
  header says why those two cases are the ones worth a test.
- `tests/transient.c` — geometry that lives one frame: an id refused by the
  frame after, the same slot drawing different contents, a static and a
  transient range drawn in one frame with the rebind between them, and an
  overrun that is refused without corrupting the frame or the next. Its header
  says why it reads the picture back rather than trusting the bookkeeping.
  Headless.
- `tests/matrix.c` — that slangc really was given `-matrix-layout-row-major`,
  checked by making a shader report a known matrix back. Its header says which
  two claims card 018 moved out of it and where they went, and why the three
  bytes it expects are the sRGB encoding of the elements rather than the
  elements.
- `tests/offscreen.c` — that back faces are culled, that the Y flip, the winding
  and the front-face constant agree about which way round that is, and that a
  texture arrives the right way up. Holds its own cube, its own camera and its
  own sun, and drives the same public calls `3d` does. Its header says why the
  sun is turned round for the mirrored case. Headless, so it runs under `ctest`
  with no window anywhere.
- `shaders/draw.slang` — the two pipelines' two entry points, the only place a
  matrix is applied to a position, the three numbers a draw finds everything by,
  and the whole of this engine's lighting. Its header says why the only push
  constant left is an object's number, and it is where every decision in the
  shading is written down: one sun and no shadows, glTF's metalness-roughness
  BRDF, what a surface with no normal looks like, why occlusion is applied where
  glTF does not put it, and which two different things take the same unlit exit
  out of the fragment stage.
- `shaders/matrix_probe.slang` — reads a matrix and writes three of its elements
  out as colour, so that a test can tell which layout slangc used.
- `vulkan/` — the Khronos headers, vendored. See `vulkan/vulkan.md`.
