# 030 — the element buffer and the GUI pipeline

status: review
claimed-by: claude-opus-5 (kanban-coder)
blocked-by: -

Written by the tech lead under the standing grant. **Not a spin-off**: it follows
029 and takes the next number. It is independent of 029 and the two can be worked
in either order, or by two people.

The decision behind this card is **ADR-0092**. Everything you need is restated
here.

## Goal

`render` can draw **many rectangles in one draw call**, from an array of small
records the graphics card expands itself.

**Solid colours only. No glyphs** — text through this path is card 031, and keeping
them apart is what makes this card verifiable on its own.

## The idea, because it is not the usual one

The normal way to draw a rectangle is to build four vertices and six indices and
send them. **This card does not do that.**

Instead the caller writes one **element record** — where the rectangle is, what
colour, and a little more — and the **vertex shader builds the four corners
itself** from the vertex index and the record. There is no vertex buffer and no
index buffer on this path at all. One instanced draw covers every element.

Two reasons, and the second is the one that decided it:

- **The CPU writes about 48 bytes per rectangle instead of four vertices**, and
  does no triangulating. That is the offload.
- **A whole GUI becomes one draw call.** Colour lives in the record rather than in
  a shading record, so every element can differ without breaking the draw. That is
  impossible on the existing path, where one draw means one colour.

**This is the same decision the engine already took once.** Card 018 could have
given each mesh its own buffers and drawn them one at a time; it took the shared
pool and the indirect-ready layout from the first mesh instead, because switching
later would have been a rewrite. Same here — and the alternative layout, real
triangles, is a rewrite of the build, the buffer and the shader.

## Scope — `render`

### The record

Its exact fields are **yours to propose** and the card will not pretend otherwise;
what it must carry is:

- **Bounds** — position and size. **In the panel's own two-dimensional space, in
  millimetres** (ADR-0089: a GUI unit is 1 mm). Not pixels, and not world metres.
- **A colour.** Straight RGBA for now. A palette index is a live question (D-144)
  and this card should not decide it — but leave the record room to grow.
- **A kind.** Solid is the only value this card ships. It exists so 031 can add the
  glyph kind without changing the record's size.
- **A clip rectangle.** Elements outside it are discarded in the fragment stage.
  **Include it now even though nothing clips yet** — it is what stops the scroll
  area on a later card from needing a draw-call break, and adding it later changes
  the record. Prove it with a test; do not leave it untested surface.

Keep it small and keep it aligned. Say in your report what you chose and what it
measures.

### The buffer

- **Host-visible and mapped, one per frame in flight**, exactly like
  `frame->objects_mapped` and `frame->uniforms_mapped` in `render/src/frame.c`. The
  fence at the top of the frame is what makes reuse safe and it is the same reason
  those two are safe. **Do not invent a second synchronisation story.**
- A new count in `voe_render_capacities` for how many elements a frame may hold.
  Fixed at device creation like everything else there.
- Overrun is a **returned refusal**, not fatal (ADR-0041), with a message naming
  the numbers — the same shape as the geometry pool's refusal.

### The pipeline

- **A second pipeline and its own shader**, not a branch in `draw.slang`. That file
  carries three uniform branches and a fourth triggers a rethink (D-093); a
  separate shader keeps well clear of it.
- **The vertex shader synthesises corners.** Six vertices per element from the
  vertex index, or four with a triangle strip — your call, say which and why.
  Nothing is read from a vertex buffer, because none is bound.
- **Blended, depth-test on, depth-write off**, matching the blended pipeline's
  rules — `render/src/device.c` is where the existing one is built and it is the
  neighbour to follow.
- **Premultiplied output** (ADR-0069). The shader multiplies colour by alpha once,
  at output. A GUI that forgets this composites wrongly against everything.
- **Order is paint order.** Primitives within one draw blend in the order they were
  submitted, so element order is what the caller sees. Say in the shader's header
  that this is relied upon.

### The draw

- One entry point that draws the frame's submitted elements, with the same
  `drawing`/assert discipline the other frame calls have.
- **Where it sits in the frame is not this card's to decide** — a panel becomes an
  entity drawn by the draw system in layer order on card 032. For this card, draw
  the elements at a point that lets you see them, and **say in your report where
  you put it and that it is provisional.**

## What must not change

State in your report that you checked each of these:

- **`voe_render_vertex` is not widened.** No colour channel, no extra field. Every
  model in the engine pays for that struct in pool space, and avoiding that tax is
  half the reason this path exists.
- **The geometry pools and their pipeline** are untouched.
- **`draw.slang`** gains nothing. Its branch count stays at three.
- **Two frames in flight** and the existing fence discipline.

## Where this card is likely to go wrong

- **Drawing with no vertex buffer bound.** This is unusual and Vulkan is content
  with it — the pipeline's vertex input state describes nothing and the shader uses
  its vertex index. If validation complains, read what it actually says before
  adding a dummy buffer.
- **Millimetres to clip space.** The buffer needs one transform for the whole
  surface. Where that comes from is card 032's problem; for now take it as a
  parameter and keep the conversion in one place with a comment, because it is the
  thing everything after this depends on being right.
- **Getting premultiplied blending subtly wrong**, which looks like a GUI that is
  slightly too faint rather than like a bug.
- **Y direction.** There is exactly one Y flip in this engine and it is in the
  viewport. Do not add a second. A GUI wants Y down and the flip may already give
  you that — work out which it is and write the answer down rather than negating
  something until it looks right.
- **Building it for the GUI specifically.** This is a general *many small things,
  one draw* path and debug lines and sprites are plausible later callers (D-146).
  Do not name anything `gui_` that does not have to be.

## Verify

- `cmake -P check.cmake` exits zero, all steps, all tests, analyser clean.
- **A headless test** — `voe_render_device_new_headless` exists for exactly this,
  and `render/tests/pools.c` is the neighbour to follow. Submit several elements of
  different colours and positions, draw, read the target back, assert the pixels.
- **A test that the clip rectangle actually clips.** It is the one field with no
  caller yet and untested surface is worse than absent surface.
- **A test that overrunning the element capacity is refused** and that the next
  frame draws correctly.
- **A dev program showing a few dozen coloured rectangles**, and a screenshot.
  Confirm with a debug label or a counter that it is **one draw call** — that is the
  claim this card exists to prove, and a screenshot alone does not prove it.
- **Confirm the 3D scene is unchanged** — cubes, sprites, sign, overlay. Nothing on
  the mesh path may have moved.
- Windows is the principal's.

## Report when this lands

- The record's fields, its size in bytes, and why.
- Six vertices or four, and why.
- Where in the frame you drew the elements, and that it is provisional.
- **The draw count for the rectangle demo, measured rather than assumed.**
- Which Y direction the element space ended up with, and how you established it.
- Whether anything about this path is GUI-specific in a way it should not be.

## Notes (coder, 2026-09-09, Linux/WSL, claude-opus-5)

**Implemented in full. `cmake -P check.cmake` exits zero — all steps, 36 tests,
analyser clean over 97 files.** Not in place, for the reasons card 029's note
already records about this machine: no `ninja`, `slangc`, `wayland-scanner` or
`pkg-config`, no sudo, and a 9p mount CMake refuses to build inside. A private
toolchain was assembled in scratch (Ubuntu `.deb`s for ninja, pkgconf and
libwayland extracted without installing, plus the Slang 2026.17 Linux release),
the tree was mirrored byte for byte onto ext4, and the unmodified script was run
on the mirror. The Khronos validation layer was fetched the same way and put in
force for every run below, **synchronization validation included** — confirmed
active from the layer's own "Current Enables" line, and **silent throughout**,
which is the answer to the card's warning about drawing with no vertex buffer
bound.

### The report the card asks for

- **The record's fields and size.** `voe_render_element` is **80 bytes**:
  `bounds` (float4, xy the top-left corner and zw the width and height, in
  millimetres), `clip` (float4, same shape), `colour` (float4, linear RGBA and
  straight, not premultiplied), `kind` (a `voe_render_element_kind`), three spare
  uints and one spare float4. Every member starts on a sixteen-byte boundary and
  `descriptors.c` asserts on the size and on four offsets, so a member that moves
  is a build error.
  **It is 80 and not the ~48 the card estimated, and the reason is card 031.**
  Three float4s plus a discriminator is 52 bytes before any padding, and the card
  requires that adding the glyph kind change no size — a glyph needs a sheet
  rectangle (four floats) and a texture index, which is 20 more bytes and does not
  fit in the 12 a 64-byte layout would leave spare. So the spare space is a float4
  and three uints, named `reserved_a`/`reserved_b` and pointed at 031, exactly as
  `voe_render_shading_values`'s `reserved_a`, `_b` and `_c` were pointed at cards
  021a, 021b and 022 and consumed in place. Against the mesh path it still makes
  the case: four `voe_render_vertex` plus six indices is 152 bytes and a
  triangulation.
- **Four vertices, not six.** A triangle strip, so two triangles per element and
  two fewer vertex stage invocations than a list. The corner is two bit
  operations on `SV_VertexID` (`vertex & 1`, `vertex >> 1`), which is exactly the
  strip's order, rather than a lookup table. The pipeline culls nothing —
  `VK_CULL_MODE_NONE` — so the strip's winding is not a fact anybody has to keep
  right and the Y flip cannot interact with it; `probe.c` makes the same choice.
- **Where in the frame the elements are drawn, and it is provisional.** There is
  one entry point, `voe_render_frame_draw_elements`, and the caller decides where
  in the frame to call it. `dev` calls it after `voe_3d_draw_system_run` and
  before `voe_render_frame_end`, so the exhibit lands over everything as a
  full-screen overlay. **That place is provisional and both `dev/src/elements.h`
  and `dev/src/main.c` say so** — card 032 makes an element surface an entity the
  draw system draws in layer order, and the call moves there then.
- **The draw count for the demo, measured.** `dev` reads
  `voe_render_frame_draw_count` either side of the exhibit and prints the
  difference, so the number is subtracted rather than asserted:

      elements   40 rectangles of 40 colours in 1 draw command; the whole frame took 30

  The headless test asserts the same thing directly — three rectangles of three
  colours, `draw_count == 1` — and a second test draws a mesh, then the elements,
  then a mesh again and requires `draw_count == 3`.
- **Which Y direction element space ended up with, and how it was established.**
  **Y runs down**: the origin is the surface's top-left corner. That is *not*
  what the engine's flip gives on its own — the negative viewport height in
  `frame.c` maps clip y = +1 to framebuffer row 0, so +Y clip space is *up* the
  screen. It was worked out from the viewport transform rather than by negating
  until it looked right, and the single negation that reconciles the two lives in
  `voe_render_element_transform` and nowhere else. **There is still exactly one Y
  flip in the engine.** Two independent checks hold it: an arithmetic one on the
  matrix ((0,0) mm → clip (-1, +1), the far corner → (+1, -1)) and a
  deliberately asymmetrical picture — three quadrants of three colours and the
  fourth left as the clear. Both were confirmed to *fail* when the sign is
  reversed.
- **Whether anything on this path is GUI-specific in a way it should not be.**
  No. Nothing is named `gui_` or `ui_`: the type is `voe_render_element`, the
  calls are `voe_render_frame_submit_element` and `_draw_elements`, and the
  header says in as many words that debug lines and sprites are plausible
  callers. The one thing that is arguably interface-shaped is that the record
  carries a *rectangle* rather than a general quad — but that is what the card
  specified and what a sprite wants too.

### What was checked against the card's "what must not change"

Each of these was checked against the diff, not from memory:

- **`voe_render_vertex` is not widened.** Untouched. The only diff line naming it
  is a sentence of the new documentation.
- **The geometry pools and their pipeline are untouched.** `geometry.c`,
  `shading.c`, `buffer.c`, `target.c`, `swapchain.c`, `texture.c` and `probe.c`
  have a zero diff. The two mesh pipelines' vertex input, topology, cull mode and
  front-face constant are unchanged.
- **`draw.slang` gains nothing.** Zero diff, branch count still three.
- **Two frames in flight and the existing fence discipline.**
  `VOE_RENDER_FRAMES_IN_FLIGHT` is unchanged and no fence, wait or submit line
  moved. The element buffer is one host-visible mapped buffer per frame slot,
  reused under the same top-of-frame fence that makes `objects_mapped` and
  `uniforms_mapped` safe; **no second synchronisation story was introduced** and
  synchronization validation reports no hazard.
- **The 3D scene is unchanged.** By the zero diffs above; by every mesh-path test
  passing, `offscreen.c` (culling, the Y flip, winding, texture orientation)
  included; by the new test drawing a mesh, the elements and a mesh again in one
  frame and asserting all three land; and by the dev program running with the
  same world and the same readout. **What was not done is a side-by-side
  screenshot of the window** — see below.

### What was added

`render` only, plus the `dev` program the card's Verify section asks for:

- `include/render/device.h` — `voe_render_element`, `voe_render_element_kind`,
  `elements` in `voe_render_capacities`, `voe_render_frame_submit_element`,
  `voe_render_frame_draw_elements`, `voe_render_element_transform` and
  `voe_render_frame_draw_count`.
- `src/element.c` — new. The third pipeline, the submit, the one instanced draw
  and the transform. The Y negation is in this file only.
- `shaders/elements.slang` — new. Four corners from a vertex index, one instance
  per element, the clip test and the one premultiply.
- `src/descriptors.c` — binding 4, one mapped element buffer per frame slot, and
  the record's size and offset asserts.
- `src/device.c` — the push constant range widened from 4 bytes to 64, and the
  element startup and shutdown calls in the right places in the order.
- `src/device_internal.h`, `src/frame.c` — the two per-frame counters and the
  per-slot buffer; `src/loader.c` — one comment brought up to date.
- `tests/elements.c` — new, headless. Seven claims, all read back out of the
  picture: four colours in one draw, the clip rectangle clipping, paint order
  both ways round, the premultiply, the capacity refusal and the frame after it,
  a mesh after an element draw, and an empty frame recording nothing.
- `dev/src/elements.h`, `dev/src/elements.c` — new. Forty rectangles: a backing
  panel, a bar twice as wide as its clip, six bars at six alphas, and
  thirty-two squares of thirty-two colours.
- `dev/src/main.c` — the two calls, the capacity, the measurement and the
  "what there is to look at" entries.

### One decision worth flagging, and two things left for the human

**A shared pipeline layout, and a push constant range widened to 64 bytes.** The
element pipeline shares `device->layout` with the two mesh pipelines. Two layouts
differing only in their push constant ranges are *not* compatible, and binding a
pipeline with an incompatible layout disturbs the descriptor set bindings — which
would have meant rebinding the set after every element draw, in a file that binds
it once at the top of the frame, and a silent wrong picture for any mesh drawn
afterwards. Sharing the layout costs the range being a matrix wide instead of a
word; the two shaders' blocks both start at offset 0 and each pipeline pushes its
own immediately before its own draw, so neither ever reads the other's bytes.
This is written down in three places (`device.c` at the range, `element.c`'s
header, `elements.slang`'s header) and **there is a test for it**, because it is
the one claim with no picture of its own.

**No screenshot of the window.** The dev program builds, runs under WSLg on
llvmpipe and prints the measurement above, with validation and sync validation
silent. But there is no screenshot tool on this machine that can capture a
Wayland client — no `grim`, `import`, `xwd` or `ffmpeg`, and the MESA screenshot
layer would not fire. So the picture was obtained instead by driving the *same*
`voe_dev_elements_submit` on a headless device at 960x540 and reading the target
back: the exhibit is correct — panel in the bottom-right, the clipped bar half
its width, the alpha ramp smooth, thirty-two distinct hues, one draw. **Looking
at the real window, and all of Windows, is the human's.** The `main.c` header
lists exactly what to check and what each failure looks like.

**Two things this card did not touch, on purpose.** A palette index instead of a
colour (D-144) is left as spare words in the record and is not decided here. The
`elements` capacity may be nought and the first submit on such a device is
refused with a message, but the per-slot buffer is still built with room for one
record — Vulkan wants every descriptor a layout declares to be valid, and 80
wasted bytes is cheaper than a conditional descriptor and a rule about when the
set may be bound. `device_internal.h` says so at the field.
