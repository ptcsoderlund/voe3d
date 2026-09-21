# src

`render`'s implementation: everything behind `include/render/device.h`, which is this folder's whole
public surface and the only header anything outside `render` includes. A reader is here to pick
which file to open — what each one owns, and where the seams between them run.

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
- `device.c` — starting up: the instance, the surface, the graphics card, the logical device, and
  the two mesh pipelines with their depth and blend state; everything that happens once, plus the
  headless device the tests run on.
- `descriptors.c` — everything the shader reads and the one layout that describes it: five bindings,
  one set, one camera buffer holding a block per pass, one object buffer and one element buffer per
  frame slot.
- `buffer.c` — a buffer with the memory under it, and the staging upload that
  fills a device-local one at an offset. Its header says why every later upload
  is this.
- `geometry.c` — the two static pools, the transient pair in every frame slot, and the ranges into
  all of them.
- `element.c` — the element path on the C side: the third pipeline, the submit that writes one
  record, the instanced draw over a range of them, and the two matrices that say what element space
  is.
- `shading.c` — the record buffer the fragment stage reads by index, and the
  slots that name its rows. Its header says why one buffer serves every frame
  slot and why creating a record waits for the GPU.
- `target.c` — the colour and depth images a frame is drawn into, one pair per frame slot; the
  targets of a caller's own, shown through one texture slot; and the read that copies a finished
  picture into an arena as RGBA8.
- `texture.c` — pixels to a sampled image: the staging copy, the layout transitions round it, the
  two samplers, and the slot table the ids name.
- `swapchain.c` — the images the window is made of, thrown away and built
  again on every resize. Nothing draws into them; they are a blit's destination.
  It is also where a requested present mode becomes the one in force, and where
  the fallback to fifo happens.
- `frame.c` — one frame and the passes in it: wait and open a recording, open a pass onto the window
  or a target with its camera, draw an object into it solid or blended, close the pass, end the
  frame and present.
- `probe.c` — the pipeline that reads a matrix and reports what it saw, built
  only when a test asks. Its header says why it is not built at startup.
