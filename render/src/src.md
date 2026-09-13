# src

`render`'s implementation: everything behind `include/render/device.h`, which is
this folder's whole public surface and the only header anything outside `render`
includes. A reader is here to pick which file to open — what each one owns, and
where the seams between them run.

`device_internal.h` is the seam itself: the struct these files share, where the
split between them runs, and the constants all of them read. Nothing here
abstracts over Vulkan — there is one graphics API and there will not be a second
— and every Vulkan call goes through the resolved function table in `loader.h`,
which is the one place in this engine where function pointers are expected.

- `loader.h` — the function-pointer table, and the one place in this engine
  where function pointers are expected. Read its header before adding to it.
- `loader.c` — opening the loader by name and filling the table in three
  passes.
- `backend.h` — the three things that differ between the two window systems,
  and the whole of what render knows about there being two.
- `backend_wayland.c` — the Linux backend. Nothing had to be done about OS
  types here and its header says why.
- `backend_win32.c` — the Windows backend, and the seven Windows typedefs
  `vulkan_win32.h` expects `windows.h` to have made. Its header says why each.
- `device_internal.h` — the struct the files below share, where the split
  between them runs, and the constants the whole folder reads.
- `device.c` — starting up: the instance, the surface, the graphics card, the
  logical device, and the two mesh pipelines with their depth and blend state. Everything that happens once,
  including the `#embed` that puts the compiled shader in the binary. Its header
  says why the three startup calls are in the order they are, and why the format
  it asks the surface for is an sRGB one now that a frame is drawn in linear
  light. Also the headless device the tests run on.
- `descriptors.c` — everything the shader reads and the one layout that
  describes it: five bindings, and one set, one camera buffer holding a block
  per pass, one object buffer and one element buffer per frame slot. Its header
  says which of the five changes how often, why the camera binding is dynamic, why writing them per frame is safe, and why the binding
  `draw.slang` does not read is in the same layout anyway.
- `buffer.c` — a buffer with the memory under it, and the staging upload that
  fills a device-local one at an offset. Its header says why every later upload
  is this.
- `geometry.c` — the two static pools, the transient pair in every frame
  slot, and the ranges into all of them. Its header says why a mesh is a range
  and not a buffer, why the indices are stored as the caller numbered them, why
  nothing static is ever freed, how the slot table splits into two bands, why
  the top-of-frame reset is the existing staleness check firing on a schedule,
  where the generation can wrap and why that is noted rather than handled, and
  why a barrier appearing in the transient path would be the bug.
- `element.c` — the element path on the C side: the third pipeline, the
  submit that writes one record, the instanced draw over a range of them, and the
  two matrices that say what element space is — one onto the surface's own plane
  in metres and one from there onto the whole target. Its header says why the
  record buffers are not in here, why the pipeline shares the other two's layout
  and what that costs the push constant range, why it is not a third variant of
  them, and why the second matrix is built on the first. The Y negation lives in
  this file and nowhere else — in one function of it. The draw's own comment says
  how a range reaches the shader and which two semantics it takes to add up.
- `shading.c` — the record buffer the fragment stage reads by index, and the
  slots that name its rows. Its header says why one buffer serves every frame
  slot and why creating a record waits for the GPU.
- `target.c` — the colour and depth images a frame is drawn into, one pair per
  frame slot, thrown away and built again on every resize.
- `texture.c` — pixels to a sampled image: the staging copy, the layout
  transitions round it, the two samplers, and the slot table the ids name. Its
  header says why there are no mipmaps and no linear filtering anywhere and what
  that costs, why the two sampling modes now differ only in addressing, and why
  there are two formats — one for a picture of a colour and one for a picture of
  numbers.
- `swapchain.c` — the images the window is made of, thrown away and built
  again on every resize. Nothing draws into them; they are a blit's destination.
  It is also where a requested present mode becomes the one in force, and where
  the fallback to fifo happens.
- `frame.c` — one frame and the passes in it: wait and open a recording, open
  a pass onto the window with its camera, draw an object into it solid or
  blended, close the pass, end the frame and present. Holds the engine's only Y
  flip and the clear colour; its header says why there are separate calls and
  not one, which pass clears the window and which load it and why no barrier
  sits between two passes, why depth is now stored, why a frame with no pass
  still clears, how each pass's camera and sun land in their own block, why the two draw calls differ
  in one argument, and why a headless device runs all but three lines of it. The
  mid-frame depth clear is the one command here recorded between draws that is
  not a draw, and its comment says why it is not a second rendering block. A draw
  may come out of either pool pair, and its header says why the pair is rebound
  only when a range's pool differs from the one last bound, and why the transient
  reset runs after the fence and nowhere else. The two timestamps that measure
  the card are written and read here, and its header says why they can only be
  read one lap late and why a reading has to be masked before it is subtracted.
- `probe.c` — the pipeline that reads a matrix and reports what it saw, built
  only when a test asks. Its header says why it is not built at startup.
