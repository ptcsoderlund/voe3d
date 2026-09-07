# 030 — the element buffer and the GUI pipeline

status: todo
claimed-by: -
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
