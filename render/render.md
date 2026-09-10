# render

The GPU, and the only folder that names Vulkan. A device opened onto a window, a
swapchain, two geometry pools and a transient pair per frame slot, textures,
shading records, three pipelines and a frame — nothing above them. Not scenes, not entities, not files, and no abstraction
over Vulkan: there is one graphics API and there will not be a second. The only
folder with shaders, and the only one slangc is run over.

Two of the three pipelines draw meshes out of the pools, one per thing drawn. The
third draws element records — many rectangles from many small records, in one
instanced draw, with no vertex buffer bound at all. A rectangle and a letter are
the same record and the same draw command on that path: an element that reads its
coverage out of a distance-field sheet is a glyph, and nothing about it is a text
system.

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
  writing into the pool. Card 030 added the element path beside all of it: its
  header says why a rectangle is a record rather than four vertices, what makes
  a whole interface one draw call, which space an element's millimetres are in
  and which way its Y runs, why the clip rectangle is there with no caller yet
  and what a zeroed one does, and how a caller reads how many draw commands a
  frame held. Card 031 spent the words that path reserved: `voe_render_element`'s
  header now says what a glyph element reads and from where, what shape the sheet
  rectangle is in and who converts to it, and what a glyph that named no sheet
  draws. Card 032 made an element draw take a range of the one buffer, so that
  one frame can hold several surfaces: its header says how a caller learns its
  own range and why that count must not be mistaken for the draw count, why a
  range past what was submitted is returned rather than asserted, which of the
  two new matrices owns the element path's Y negation and why the other is built
  on it rather than beside it, and — on the call that turns pixels per millimetre
  into a surface's millimetre size — why that number is a parameter and why an
  authored millimetre means one thing on a surface filling the window and another
  on one standing in the world.
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
  logical device, and the two mesh pipelines with their depth and blend state. Everything that happens once,
  including the `#embed` that puts the compiled shader in the binary. Its header
  says why the three startup calls are in the order they are, and why the format
  it asks the surface for is an sRGB one now that a frame is drawn in linear
  light. Also the headless device the tests run on.
- `src/descriptors.c` — everything the shader reads and the one layout that
  describes it: five bindings, and one set, one camera buffer, one object buffer
  and one element buffer per frame slot. Its header says which of the five
  changes how often, why writing them per frame is safe, and why the binding
  `draw.slang` does not read is in the same layout anyway.
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
- `src/element.c` — the element path on the C side: the third pipeline, the
  submit that writes one record, the instanced draw over a range of them, and the
  two matrices that say what element space is — one onto the surface's own plane
  in metres and one from there onto the whole target. Its header says why the
  record buffers are not in here, why the pipeline shares the other two's layout
  and what that costs the push constant range, why it is not a third variant of
  them, and why the second matrix is built on the first. The Y negation lives in
  this file and nowhere else — in one function of it. The draw's own comment says
  how a range reaches the shader and which two semantics it takes to add up.
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
- `tests/elements.c` — rectangles and letters from records: four colours in one
  draw command read back out of the picture, that the clip rectangle really
  clips, that submission order is paint order both ways round and across the two
  kinds, that the blend multiplied by alpha exactly once, that overrunning the
  element capacity is refused without spoiling the frame or the next, that a mesh
  drawn after two element draws is still drawn right, that a solid and a glyph in
  one frame are one draw, that two ranges of the one buffer land where their own
  two matrices say, that a range past what was submitted is refused with the
  frame left intact, and that a screen-filling surface's corners come out
  strictly inside the near clip boundary rather than on it. Its header says why
  the arrangement is deliberately asymmetrical, why the draw count is counted rather than assumed, which of the
  claims has no picture of its own, why the sheet is hand-made rather than a
  font, what a glyph that named no sheet draws, and why the range test moves only
  the matrix. Five of its claims need no graphics card: the two matrices'
  directions, the near clip boundary, the surface size, and the record's size.
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
- `shaders/elements.slang` — the element pipeline's two entry points: four
  corners built out of a vertex index, one instance per rectangle, a clip test in
  the fragment stage, and the sheet a glyph reads its coverage out of. Its header
  says why it is a second shader rather than a fourth branch in `draw.slang`, why
  nothing is read from a vertex buffer, why four vertices and a strip rather than
  six, why the record's number takes two semantics added together and what
  reading only one of them looks like, that order is paint order and what relies
  on it, why its push constant
  aliases `draw.slang`'s, why the threshold is a threshold and not a smoothstep,
  why the sheet is sampled at an explicit level, why its texture index is
  non-uniform where `draw.slang`'s is not, and that it holds a second copy of the
  median.
- `shaders/matrix_probe.slang` — reads a matrix and writes three of its elements
  out as colour, so that a test can tell which layout slangc used.
- `vulkan/` — the Khronos headers, vendored. See `vulkan/vulkan.md`.
