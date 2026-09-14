# 0092. The GUI is one draw call from an element buffer the GPU expands

- **Status:** Accepted
- **Date:** 2026-09-07
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Closes:** D-143

## Context

The principal, unable to choose between ADR-0091's options and reaching for the
performance philosophy to break the tie:

> Our philosophy is performance as default. We want to offload as much as possible
> to gpu and have as few drawcalls as possible. Can we reduce whole gui to one
> drawcall? Should it be vector based? Or just somehow have a vertexbuffer or
> similar for data where one gui shader can batch everything?

**The first finding is that this does not break the tie, because it is a separate
decision.** Immediate mode and a retained tree both produce the same thing — an
ordered list of rectangles to paint. One fills it by walking nested calls, the
other by walking a tree. **The renderer downstream is identical**, so draw count
cannot discriminate between them. The philosophy decides *this* ADR, and ADR-0091
has to be decided on its own merits.

The coupling that does exist is negligible and worth dismissing explicitly:
immediate mode rebuilds the list every frame, a retained tree could upload once.
At GUI scale that is ~256 KB a frame against memory that takes gigabytes a second
— ADR-0084's arithmetic, roughly a tenth of a millisecond.

Constraints already fixed:

- **ADR-0016 — GPU-first**, and its stated consequence: entity data in GPU buffers
  indexed by id, indirect draws from GPU-written commands. **Architected from the
  start, not retrofitted.**
- **ADR-0059 — the precedent that governs this.** Card 018 faced the same shape and
  chose *indirect-ready layout, plain draws*: pick the data layout the destination
  needs from the first mesh, issue it the simple way until something measures.
  Switching later *"changes the draw loop and nothing else."*
- **ADR-0018 — resources by id, descriptor indexing and buffer device address are
  core in the Vulkan 1.3 floor.** Bindless is available.
- **ADR-0088 — the style is Windows 10: flat, square, solid fills.**
- **ADR-0078 — no anti-aliasing.** **ADR-0089 — authored in millimetres.**
- **ADR-0055 / ADR-0034** — nothing built that nothing measures; implement on
  demand.
- **D-132** — the row that asked this in a different costume, and which this closes
  in substance.

## Why one draw is not possible today

Two independent blockers, both established while answering the principal's earlier
question:

1. **Colour lives in the shading record, not on the vertex.** `voe_render_vertex`
   is `{position, normal, uv}`. One draw is one shading record, therefore one
   colour.
2. **`base_colour_distance_field` is per shading record too.** Glyphs are a
   distance field and solid fills are not, so **text and panel geometry cannot
   share a draw at all**, whatever is done about colour.

A Windows-10-shaped panel is therefore five to ten draws. That is noise against
ADR-0081's thousand-draw budget — but the question was whether it can be one, and
the answer needs a change.

## Options considered

### Option A — triangles with a richer vertex

One vertex buffer for the whole GUI; each vertex carries position, uv, a colour or
palette index, and a flag for how to shade it. One pipeline, one draw. Dear ImGui's
model, plus bindless to remove the per-texture break.

- **Costs:** a GUI vertex is genuinely not a mesh vertex — it needs a colour and a
  kind, and it has no use for a normal. So either `voe_render_vertex` widens and
  **every model in the engine pays for it in pool space**, or the GUI gets a second
  vertex format and a second pool. ADR-0059 built exactly one shared pool.
- **Makes easy:** it is the well-trodden path and the shader is trivial.
- **CPU cost:** four vertices per rectangle, written every frame.

### Option B — vector, evaluated in the fragment stage

Draw a quad per element and evaluate the shape analytically — signed distance
functions for rectangles, rounded corners, strokes, curves.

- **Makes easy:** resolution independence, which speaks directly to ADR-0089's
  millimetres and to D-137's sub-pixel hairline. Rounded corners and strokes come
  free.
- **Costs:** every covered pixel does shape maths, where a triangle rasteriser
  fills for nothing. Overdraw multiplies it. And D-093 is watching `draw.slang`'s
  branch count, though a separate GUI shader sidesteps that.
- **The deciding objection: the style is square.** ADR-0088 chose Windows 10 — flat,
  square corners, solid fills. **Vector's entire advantage is curves**, and this
  interface has rectangles and text, where text is *already* a distance field with
  its own path. **The principal's own style choice removes most of the reason to
  do this.**

### Option C — an element buffer the GPU expands

The CPU writes an array of **element records** — bounds in millimetres, a colour
or palette index, a kind, a texture id, a uv rect, a clip rect. No vertex buffer at
all: the vertex shader synthesises the four corners from the vertex index and the
record. One instanced draw for the whole GUI.

- **Makes easy:** genuinely one draw call. **Text and solid fills unify**, because
  *which kind of thing this is* moves from the shading record into the element
  record where it can vary per element — which is what removes both blockers above.
  Clipping for scroll areas becomes a rectangle in the record and a discard, rather
  than a scissor change and a draw break.
- **CPU cost is the real win:** ~48 bytes per element instead of four vertices,
  and no triangulation. That is the *offload* the principal is asking for, and it
  falls on the CPU side where the cost actually is.
- **Ordering is safe.** Primitives within one draw blend in order, so instance
  order is paint order — which is what a GUI needs and what ADR-0061's blended pass
  already relies on.
- **Costs:** a second pipeline and a second shader in `render`, and a buffer type
  that does not exist. More machinery than Option A.

## Decision

**Option C, and the principal took it. ADR-0059 is the precedent rather than an analogy.**

Card 018 stood exactly here: the simple thing was per-mesh buffers and one draw
each, the destination was a shared pool with indirect draws, and the decision was
to **take the destination's data layout from the first mesh and issue it the simple
way until something measured.** The reasoning was ADR-0016's — the retrofit is what
GPU-first exists to avoid.

The same applies, and more sharply, because **Option A and Option C have different
data layouts.** Triangles-now-elements-later is a rewrite of the build, the buffer
and the shader. Elements-now is not more work than triangles-now by much, and it is
the layout the destination needs.

**On the two sub-questions directly:**

- **Can the whole GUI be one draw call? Yes**, under Option C, including text.
- **Should it be vector based? No** — and the reason is the principal's own style
  choice. Vector earns its place with curves; Windows 10 is rectangles, and the one
  curved thing in the interface is text, which already has a distance-field path
  built and tuned. **Icons are the one place it might return**, and that is a later
  question with a real trigger.

## What this pins

1. **The GUI's unit of drawing is an element record**, not a triangle. Bounds,
   colour or palette index, kind, texture, uv rect, clip rect.
2. **The GPU expands records into quads.** No GUI vertex buffer, and
   `voe_render_vertex` is **not** widened — every model keeps its pool space.
3. **One instanced draw for the whole GUI**, text included. Paint order is instance
   order.
4. **Clipping is per element**, in the record, not a scissor break.
5. **`render` gains a second draw path** — *draw N elements from this buffer* —
   which is a general capability, not a GUI-only one. Debug lines and sprites are
   plausible later callers.
6. **It is drawn the simplest correct way first.** A GPU-written or culled element
   list is a later card with its own trigger, exactly as ADR-0059 left the indirect
   command buffer unbuilt.

## Blast radius

**Moderate, and deliberately paid early.** The element record's shape is the
load-bearing part: once the GUI builds them and a shader reads them, changing the
record changes both. That is the cost ADR-0059 accepted for meshes and for the same
reason.

What stays cheap: how the draw is *issued*. Instanced now, indirect later, culled
later still — each of those changes the draw loop and nothing else.

What this avoids: widening `voe_render_vertex` for every mesh in the engine to
serve a GUI, which is the version of this that would have been expensive and
permanent.

Reversibility: **moderate.**

## Consequences

- **D-132 closes in substance.** A palette is now one of two ways an element names
  its colour, and the choice is inside this record rather than a separate decision.
- **The GUI needs something new from `render` after all**, which reverses a claim in
  ADR-0088. That ADR said the style needs nothing new; true of *triangles and
  colours*, false once one draw call is the goal. **The claim was about the style
  and this is a choice about performance** — the style still demands nothing.
- **A second pipeline and shader** in `render`, outside `draw.slang`, so D-093's
  branch count is untouched.
- **This is more machinery than the first GUI card needs**, and that tension is
  real: ADR-0055 says nothing is built that nothing measures. The answer is
  ADR-0059's — the *layout* is chosen ahead of the measurement, the *mechanism* is
  not.
- **The consequence I do not like:** this is a performance architecture chosen
  before a single widget exists, on a philosophy rather than a number. ADR-0059 did
  the same thing and was right, but it had a card in front of it and this does not.
  If the GUI cards are cut and this turns out to be scaffolding nobody needed, that
  is a fair criticism of this ADR and not of the philosophy.

## Rejected options and why

**Option A — richer vertices.** The industry-standard answer and a fine one, but it
either taxes every mesh in the engine or forks the vertex pool, and it is a
different data layout from the destination, so it buys a rewrite.

**Option B — vector.** Rejected on the style. Windows 10 is square and flat; vector
pays for curves this interface does not have. Revisit only for icons.

**Doing nothing until measured.** The honest alternative, and the one ADR-0055
argues for. Rejected on ADR-0059's precedent: the data layout is the expensive half
and it is chosen now precisely so it never has to be retrofitted.

## Questions this opens

- **D-144 — the element record's exact fields**, and whether colour is inline or a
  palette index. Trigger: the first GUI card. This is the field D-132 was really
  asking about.
- **D-145 — whether icons bring vector back.** An icon set is the one part of a
  Windows 10 interface that is curves, and the alternatives are a distance-field
  sheet like the glyph atlas or plain textures. Trigger: the first icon.
- **D-146 — whether the element path is offered beyond the GUI.** Debug lines and
  sprites want the same *many small things, one draw* shape, and if it generalises
  it should not be named for the GUI. Trigger: the second caller.
