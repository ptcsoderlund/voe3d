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
  geometry, textures and shading records and get ids back, make targets of its
  own, show their pictures through a texture id and read any target's picture
  back into memory as RGBA8, then begin a frame, open
  passes onto the window or a target each with a camera and a sun or with none, draw
  objects into a pass solid or blended, submit elements and draw ranges of them,
  clear the depth part-way through, and end it. Geometry comes in two lifetimes, a caller says at startup which present
  mode it wants, and when a frame is over it can ask how long the card spent on
  it and how many draw commands it held. Its header says why nothing here knows
  what a scene is, why an id's index half is the number the shader uses, why
  geometry lives in two shared pools, what the padded records are padded for,
  why a colour texture and a data texture are not interchangeable, why a
  texture's kind and its sampling mode are two independent questions, why the
  sampling modes' names now say more than they mean, why exactly one of them
  filters and what a texture is claiming by asking for it, and why an object
  carries a normal matrix as well as a world one. On a frame: why the camera
  belongs to the pass and may be missing, which pass clears the window and which
  load it, that passes do not nest, why a target is kept rather than asked for
  per frame, why its texture id holds across frames in flight and resizes, when
  its picture is undefined, that showing a target in the pass drawing into it is
  a debug assert; on reading one back: why the channel swap and the divide by
  alpha are this folder's, which frame's picture comes back, and why it waits for
  the card and is therefore not a per-frame call; and why element submission is frame-wide
  while element drawing is per pass; what the three
  alpha modes mean, why the depth write goes off for a blended draw and the
  order therefore matters, that the colour target holds premultiplied colour,
  why the mid-frame depth clear takes no clear value and what the colour
  attachment does while it happens, why the frame timing is its own clock and
  runs two frames behind, and why mailbox is optional and falling back to fifo
  is not a failure. On the two geometry lifetimes: which create is a startup
  operation and which lives inside a frame, why one id type serves both, and why
  the caller builds the arrays rather than writing into the pool. On a shading
  record: why not being lit is a property of a surface rather than of a pass,
  why the rectangle of the base colour texture a record reads is what puts a
  sheet of frames behind one geometry, and why the whole texture is not a zeroed
  rect. On the element path: why a rectangle is a record rather than four
  vertices, what makes a whole interface one draw call, which space an element's
  millimetres are in and which way its Y runs, why the clip rectangle is there
  with no caller yet and what a zeroed one does, how a caller reads how many
  draw commands a frame held, what a glyph element reads and from where, what an image element shows, what
  shape the sheet rectangle is in and who converts to it, what a glyph that
  named no sheet draws, how a caller learns its own range and why that count
  must not be mistaken for the draw count, why a range past what was submitted
  is returned rather than asserted, which of the two matrices owns the element
  path's Y negation and why the other is built on it rather than beside it, and
  — on the call that turns pixels per millimetre into a surface's millimetre
  size — why that number is a parameter and why an authored millimetre means one
  thing on a surface filling the window and another on one standing in the
  world.
- `src/` — the implementation behind that header: the loader and the two window
  backends, startup, the pools, the three pipelines and the frame. See
  `src/src.md`.
- `tests/` — eight programs, most of them headless, that check what reached the
  picture rather than what the bookkeeping said. See `tests/tests.md`.
- `shaders/` — the three Slang shaders, compiled and embedded at build time. See
  `shaders/shaders.md`.
- `vulkan/` — the Khronos headers, vendored. See `vulkan/vulkan.md`.
