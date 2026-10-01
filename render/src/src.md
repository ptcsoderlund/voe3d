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
- `device_internal.h` — the device struct the files below share, the calls between them, where
  the split runs, and the constants the whole folder reads.
- `device_parts.h` — the records the device is built of: buffers, pools, slots, targets and the
  one-frame record; included only through `device_internal.h`.
- `device.c` — starting up, and the one place its order is: the surface, the logical device, the
  format, timing, present modes, frame objects and close-down, plus the headless device the tests
  run on.
- `startup.h` — the startup steps that live beside device.c, why open_device calls them in the
  order it does, and the card facts and ranking a test can call with no card.
- `instance.c` — the Vulkan instance, its extensions, and the validation layer and messenger in a
  debug build that has them.
- `card.c` — ranking the graphics cards by kind then memory, choosing one, and the `render` line
  that says which and why.
- `pipeline.c` — the four mesh pipelines, solid, blended, shadow and bounce, with their embedded shader, depth and blend state, and the
  layout every pipeline shares.
- `descriptors.c` — everything the shader reads and the one layout that describes it: seven bindings,
  one set, one camera buffer holding a block per pass, one object buffer, one element buffer and the
  shadow maps per frame slot, and every bounce grid with its trilinear repeat sampler.
- `buffer.c` — a buffer with the memory under it, and the staging upload that
  fills a device-local one at an offset. Its header says why every later upload
  is this.
- `geometry.c` — the two static pools, whose ranges are freed and reused first fit, the transient
  pair in every frame slot, and the ranges into all of them.
- `element.c` — the element path on the C side: the third pipeline, the submit that writes one
  record, the instanced draw over a range of them, and the two matrices that say what element space
  is.
- `shading.c` — the record buffer the fragment stage reads by index, and the
  slots that name its rows, freed and reused. Its header says why one buffer
  serves every frame slot and why creating a record waits for the GPU.
- `target.c` — the colour and depth images a frame is drawn into, one pair per frame slot, each
  depth with its sampled copy, the image build, settle and teardown every target shares, and the
  bounce grids, built cleared.
- `target_own.c` — the targets of a caller's own: their table, create, resize, and the settle into
  GENERAL that lets one texture slot show them, and a second slot for their depth copy; each keeps its bounce grid through a resize.
- `target_read.c` — the read that copies a finished picture, the window's or a caller's target,
  into an arena as RGBA8 with straight alpha.
- `shadow.c` — the sun's shadow maps: one D32 array image of four cascades per frame slot, its
  views, the barriers either side of a shadow pass, and the comparison sampler they are read through.
- `bounce_map.c` — the sun's bounce map: per frame slot a D32 depth and RGBA16F flux and normal
  images, 512 square, the bounce pass's rendering, and its barriers to compute read.
- `bounce_schedule.h` — which bounce probes an update refreshes and how much: entered cells, then
  stale spheres, then a strided cycle, in toroidal indices. Pure CPU.
- `bounce_schedule.c` — that schedule's one call, its bit set against listing an index twice.
- `texture.c` — pixels to a sampled image: the staging copy, the layout transitions round it, the
  two samplers, and the slot table the ids name.
- `swapchain.c` — the images the window is made of, thrown away and built
  again on every resize. Nothing draws into them; they are a blit's destination.
  It is also where a requested present mode becomes the one in force, and where
  the fallback to fifo happens.
- `frame_internal.h` — the calls frame.c, pass.c, draw.c and present.c make across one another;
  included by those four only.
- `frame.c` — one frame: wait for the slot and open a recording, read the GPU time it measured,
  rebuild on resize, end, submit and present.
- `pass.c` — a pass: one rendering block onto the window or a target with its camera block, the
  clear colour, the first-clears-later-load rule, the depth copy, and the one Y flip in the viewport.
- `draw.c` — the draws inside a pass: one object record per mesh draw, solid or blended, the depth
  clear between them, and the rebinds only when pipeline or pool pair changes.
- `present.c` — the last thing a frame records: the target made ready to copy, and the blit that is
  the one write into a swapchain image.
- `probe.c` — the pipeline that reads a matrix and reports what it saw, built
  only when a test asks. Its header says why it is not built at startup.
