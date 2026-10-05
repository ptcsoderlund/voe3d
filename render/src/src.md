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
- `device_internal.h` — the device struct the files below share, where the split runs, and the
  constants the whole folder reads.
- `device_calls.h` — the calls one file here makes into another, grouped by the file that owns
  each; included only through the end of `device_internal.h`.
- `device_parts.h` — the records the device is built of: buffers, pools, slots, targets, the
  one-frame record and a pass's blocker region with its kinds; included only through
  `device_internal.h`.
- `device.c` — opening, and the one place its order is: the surface, the logical device, the
  format, timing, present modes, frame objects, the layout and element pipeline but no mesh
  pipeline, and close-down, plus the headless device the tests run on.
- `startup.h` — the startup steps that live beside device.c, why open_device calls them in the
  order it does, and the card facts and ranking a test can call with no card.
- `instance.c` — the Vulkan instance, its extensions, and the validation layer and messenger in a
  debug build that has them.
- `card.c` — ranking the graphics cards by kind then memory, choosing one, and the `render` line
  that says which and why.
- `pipeline.c` — the layout every pipeline shares, made at open, and prepare, which builds the five
  mesh pipelines, solid, blended, shadow, point shadow and capture, then the relight, a step a call.
- `descriptors.c` — everything the shader reads and the one layout that describes it.
- `records_layout.c` — the build-time proof that the C and Slang layouts of every record a shader
  reads agree, by size and member offset; no code.
- `buffer.c` — a buffer with the memory under it, and the staging upload that
  fills a device-local one at an offset. Its header says why every later upload
  is this.
- `geometry.c` — the two static pools, whose ranges are freed and reused first fit, the transient
  pair in every frame slot, and the ranges into all of them, each with its bounding sphere and its
  vertex box, which `voe_render_geometry_box` hands back.
- `element.c` — the element path on the C side: the third pipeline, the submit that writes one
  record, the instanced draw over a range of them, and the two matrices that say what element space
  is.
- `shading.c` — the record buffer the fragment stage reads by index, and the
  slots that name its rows, freed and reused. Its header says why one buffer
  serves every frame slot and why creating a record waits for the GPU.
- `target.c` — the colour and depth images a frame is drawn into, one pair per frame slot, each
  depth with its sampled copy, the image build, settle and teardown every target shares.
- `target_own.c` — the targets of a caller's own: their table, create, resize, and the settle into
  GENERAL that lets one texture slot show them, and a second slot for their depth copy.
- `target_read.c` — the read that copies a finished picture, the window's or a caller's target,
  into an arena as RGBA8 with straight alpha.
- `shadow.c` — the directional lights' shadow maps: one D32 array image of four cascades per held
  light per frame slot, its views, its growth to up to four lights at the top of a frame, the
  barriers either side of a shadow pass, and the comparison sampler they are read through.
- `point_shadow.c` — the point lights' shadow maps: one D32 array image of 6 × 16 layers per frame
  slot, its sampled and attachment views, settled to shader-read, whether they are ready, and the
  layered point-shadow pass onto every layer with its lights by slot.
- `bounce_probes.h` — the captured bounce grid's bookkeeping: the toroidal 24 × 12 × 24 index,
  which probes hold a picture or are queued, the bouncing lights, every sun among them, and when to
  relight. Pure CPU.
- `bounce_probes.c` — its place at a grid's own spacing, nearest-first take, relight-needed (lamps
  and light blockers about the corner, so an eye that moves relights nothing; a change of kinds or
  the sun's mask relights) and relit calls over bit sets.
- `bounce_volume.c` — a target's probe volume: its atlases and six-axis irradiance 3D images, built
  at the top of the frame after the first `voe_render_bounce_begin` and named at bindings 6 and 10,
  freed after 300 frames unbegun, and that begin, which keeps its spacing per frame slot.
- `bounce_capture.c` — the capture pass: per frame slot a 96-layer scratch of albedo, normal and
  depth, sixteen queued probes drawn into it as one layered pass at the volume's spacing and out
  to its reach, twelve cells, then copied into the atlases.
- `bounce_shadow.c` — the relight's own sun maps: per frame slot a 1024-texel D32 image of a layer per sun,
  and the shadow pass that draws one sun's layer once per bounce begin that relights it.
- `bounce_relight.c` — the relight: its three compute pipelines, set layout, pool, per-slot probe
  lists and one record per volume with the begun light blockers, their kinds and the sun's mask,
  and the call that settles changed
  probes, relights each level in use and sums them.
- `light_bins.h` — which of 16 × 9 screen tiles and 32 exponential depth slices each point light of
  a pass reaches, one bit per light in each, and the slice of a view distance. Pure CPU.
- `light_bins.c` — those two calls: a light's view-space sphere to its slices and to the NDC
  rectangle of its box's corners.
- `light_blockers.c` — `voe_render_light_blockers_mask`, declared in `render/device.h`: the boxes
  holding a point, bit i for blocker i, a sphere compare, then three rows, inclusive at the face.
- `point_shadow_faces.h` — a caster's bounding sphere from its vertices, moved under a world
  matrix, and the 6-bit mask of a point light's cube faces it reaches, +X −X +Y −Y +Z −Z. Pure CPU.
- `point_shadow_faces.c` — those three calls: a box-centred sphere, its move under a world matrix,
  and the face mask against one light's widened pyramids; plain arithmetic, no state.
- `texture.c` — pixels to a sampled image: the staging copy, the layout transitions round it, the
  two samplers, and the slot table the ids name.
- `swapchain.c` — the images the window is made of, thrown away and built
  again on every resize. Nothing draws into them; they are a blit's destination.
  It is also where a requested present mode becomes the one in force, and where
  the fallback to fifo happens.
- `frame_internal.h` — the calls frame.c, pass.c, depth_copy.c, draw.c, present.c, point_shadow.c,
  bounce_capture.c and bounce_shadow.c make across one another; included by those eight only.
- `frame.c` — one frame: wait for the slot and open a recording, read the GPU time it measured,
  rebuild on resize, end, submit and present.
- `pass.c` — a pass: one rendering block onto the window or a target, its camera block with probe
  volume, point lights, light blockers and further lights, the clear colour, the
  first-clears-later-load rule, and the one Y flip.
- `depth_copy.c` — the depth copy (ADR-0305): a camera pass's block split in two round a copy of
  its depth into the sampled copy beside it, the second block loading, and the copy's slot written
  into the pass's block.
- `draw.c` — the draws inside a pass: one object record per mesh draw, solid or blended, and in
  the point-shadow pass one instance per cube face reached, the depth clear between them, and the
  rebinds only when pipeline or pool pair changes.
- `present.c` — the last thing a frame records: the target made ready to copy, and the blit that is
  the one write into a swapchain image.
- `probe.c` — the pipeline that reads a matrix and reports what it saw, built
  only when a test asks. Its header says why it is not built at startup.
