# 0084. Geometry that changes is a transient pool, reset every frame, and the engine caches nothing

- **Status:** Accepted
- **Date:** 2026-09-07
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Closes:** D-092

## Context

Everything this engine draws is a mesh uploaded once.
`voe_render_geometry_create` waits for the graphics card to go idle, stages the
bytes through a temporary buffer, and appends to a pool that has no destroy
(`render/src/geometry.c`, ADR-0059). That is correct for a model and fatal for a
label: a text mesh rebuilt every frame would stall the card and then exhaust the
pool.

Card 021b built text on that path and said so on its face — *a text block is
built once and does not change* — and named the consequence: the frame statistics
are still printed on the console rather than drawn.

**Three consumers are blocked, and only one of them is text.**

- The frame statistics readout (card 020 left it on the console for this reason).
- **Card 023, the GUI.** A GUI whose labels are fixed at startup is not a GUI.
  This is that card's oldest unmet dependency and the reason it is unclaimable.
- Any debug drawing at all — a line, a box round something, a gizmo — has no path
  either, and never has had.

Constraints already fixed:

- **ADR-0070 — one text block is one mesh, one material, one draw.** Load-bearing:
  it is what keeps a sentence from becoming a hundred blended objects with a
  hundred sort keys.
- **ADR-0059 — a mesh is a range in shared pools**, and the pools are bump
  allocated and never freed. **D-068** — what to do about the hole a removal
  leaves — is open and deferred to the first card that unloads a model.
- **ADR-0050 — two frames in flight.** Bytes the card may still be reading cannot
  be overwritten.
- **ADR-0081, 2026-09-06 — the scale bundle is deferred.** D-067, D-071 and D-114
  are one decision with one trigger: the first card that genuinely draws hundreds
  of objects with real content behind it. D-114 names one route out of this
  decision — a quad per glyph — as a thing that would fire that bundle early.
- **ADR-0020** — GPU work is justified, not default. **ADR-0055** — nothing is
  built that nothing measures. **ADR-0034** — implement on demand.
- **ADR-0060** — `render`'s public surface is by id and grown one caller at a
  time.

**The machinery already exists in a smaller form.** Each frame slot already owns a
host-visible, mapped buffer that is written fresh every frame — the uniforms and
the per-object records (`render/src/frame.c`). "Written by the CPU during the
frame, one copy per slot in flight" is a proven shape in this folder, not a new
idea.

## Options considered

### Option A — a transient pool: geometry that lives one frame

A second pair of pools, host-visible and mapped, **one set per frame in flight**,
reset to empty at the top of every frame. A caller writes vertices and indices
into it mid-frame and gets back an ordinary `voe_render_geometry` id, drawn
through the existing `voe_render_frame_draw` and `_draw_blended`. The id is
refused after its frame by the generation check that already guards every id
(`voe_render_geometry_at`).

No idle wait, no staging copy, no descriptor rewrite, and nothing to exhaust —
the reset is what makes exhaustion impossible.

- **Costs:** one more pool kind, a per-frame capacity number in
  `voe_render_capacities`, and a genuinely new idea in the public surface —
  geometry that expires. Host-visible memory is marginally slower for the card to
  read than device-local; at these sizes it is noise.
- **Makes easy:** everything that changes — text, the statistics readout, debug
  lines, a GUI panel whose contents moved. One mechanism, not one per consumer.
- **Makes permanent:** two lifetimes of geometry in the public API, forever.
  Which is honest: the distinction is real, and the alternative to naming it is
  the version where a per-frame call silently stalls.
- **Prior art:** the standard transient/dynamic vertex path. Dear ImGui's entire
  model is this and nothing else; bgfx names it `allocTransientVertexBuffer`.

### Option B — one quad per glyph, fed from a per-frame buffer

Upload a unit quad once. Every frame write a record per glyph — where, how big,
which patch of the sheet, what tint — and issue one instanced draw per block.

- **Costs:** this is instancing, which is on the *later* list by name, and
  ADR-0081 records that taking this route here is what fires the deferred scale
  bundle. It needs a new input path and a shader reading per-glyph records,
  against D-093's watch that `draw.slang` is one uniform branch from wanting to be
  split.
- **Makes easy:** text specifically, and well — at a thousand glyphs it is the
  better shape.
- **Solves nothing else.** The statistics readout is text, so it is served; a
  debug line and a changing GUI panel are not.

### Option C — rewrite a mesh in place

Reserve the worst case at creation, then overwrite the range when the string
changes.

- Dies on the constraints. With two frames in flight the bytes cannot be
  overwritten while the card may still be reading them. Fixing that needs a copy
  per frame slot — which is Option A's ring, with the caller doing the bookkeeping
  and a fixed maximum bolted on. The version that avoids the hazard is the one
  that waits for idle, which is the stall being escaped, hidden behind a call that
  looks cheap.

### The sub-question the principal raised: why not cache until something changes?

Asked directly, and it changed the shape of the answer without changing the
option. Three findings:

1. **The static pool already is the cache.** Text fixed at startup stays an
   ordinary uploaded mesh — the path card 021b built — and costs one upload
   forever. Nothing here takes that away or asks anyone to stop using it. The
   question is only about text that changes *sometimes*.
2. **An engine-side cache has nowhere to live.** Not the transient pool, which is
   reset by construction. Not the static pool, which is Option C. A third
   persistent-but-mutable pool inherits D-068 — what to do with the hole when
   something shrinks or goes away — and text is not the evidence on which to
   answer that.
3. **The failure mode is worse than the cost, and the cost is small.** A cache
   that misses an invalidation draws yesterday's text: a bug that looks like the
   program has frozen, found by staring rather than by anything failing. Against
   that: two thousand glyphs on screen — a dense GUI — is eight thousand vertices,
   about 256 KB written per frame, some 15 MB a second at sixty frames. A copy
   into mapped memory runs in the gigabytes per second. Call it a tenth of a
   millisecond, against the 1–3 ms per thousand objects that *submitting* the draw
   calls already costs by ADR-0081's number. **Streaming is not the binding cost;
   submission is** — the same answer, for the same reason, as the sort in
   ADR-0081.

## Decision

**Option A, and the principal chose it. The engine caches nothing.**

The deciding factor: **the thing that has to change is not text, it is geometry
produced this frame.** Three consumers are already named and only one is text;
Option A serves all three with one mechanism, and Option B serves one and leaves
the other two to invent something.

What this pins:

1. **A transient pool exists**, host-visible and mapped, one set per frame in
   flight, reset at the top of every frame. Vertices and indices both.
2. **It hands back an ordinary `voe_render_geometry` id.** No second id type and
   no second pair of draw entry points. An id used after its frame is refused by
   the staleness check that is already built and tested — one draw call, one id
   type, no new refusal path.
3. **Fixed text keeps the static pool.** Uploaded once, drawn forever, unchanged
   by this decision.
4. **There is no engine-side cache of geometry, and there will not be one added
   quietly.** Building one is a decision, not an optimisation.
5. **The caller may cache its own work, in ordinary memory, with no help from
   here.** If laying the string out costs more than streaming the bytes, `text`
   keeps the vertices it built and re-streams them unchanged. That skips the
   layout, keeps the copy, has no GPU hazard, and its failure mode is local and
   visible. It needs nothing from `render` and no ADR — it is an optimisation
   inside one folder, built when something measures it, under ADR-0055.
6. **The GUI's real cache is the panel, not the vertices.** A complicated panel is
   drawn once into its own offscreen target and composited as a single quad, which
   is already card 023's plan. The heavy case caches *pixels*; caching vertices
   underneath it would be caching the cheap half.
7. **The scale bundle is not fired.** A transient pool is orthogonal to instancing
   and indirect draws. D-067, D-071 and D-114 keep their trigger untouched.

## Blast radius

**Moderate.** The pool itself is internal and its capacity is a number, but the
public surface gains a permanent idea — a lifetime — and ADR-0060 says that
surface grows on demand and does not shrink again. Reversing this would break
every caller that had come to rely on drawing something it built this frame,
which after card 023 is expected to be most of the GUI.

What stays cheap: nothing structural is foreclosed. Per-glyph instancing, if it
ever earns its place, is built **on top of** a transient buffer rather than
instead of it, and the scale bundle is untouched.

The thing that could go wrong: the transient pool becoming the path of least
resistance for geometry that never changes, because it is the one that cannot
fail at startup. A model streamed every frame would work, look correct, and waste
the bandwidth of the whole scene. See the consequences.

Reversibility: **moderate.**

## Consequences

- **Card 023 unblocks.** Its oldest unmet dependency is answered, and what remains
  between it and the board is that nobody has cut it into cards — a separate job.
- **The statistics readout can leave the console**, which is where card 020 said
  it would go once this existed.
- **Debug drawing becomes possible for the first time.** Not built here; the path
  exists after this.
- **The public API grows a second lifetime**, and every reader of `device.h` now
  has to know which one they are holding. The id type does not distinguish them,
  which is what keeps the draw path single — and is therefore also the one place
  a caller can be wrong without the compiler helping. The staleness check catches
  it at the next frame, not at the mistake.
- **A per-frame capacity has to be chosen**, and a caller that overruns it is
  refused mid-frame — a new failure point in a place that previously could not
  fail. Refusal is returned, not fatal, under ADR-0041.
- **Host-visible geometry is slower for the card to read.** Irrelevant at a
  GUI's sizes and not irrelevant at a scene's, which is exactly the misuse named
  in the blast radius.
- **We accept re-laying-out unchanged strings** until something measures that it
  matters, and the measurement is `text`'s to take.
- **The consequence I do not like:** this is the first place where the right
  answer for a big rare-changing mesh — a terrain chunk, a deformed character — is
  neither pool. Those will want persistent-mutable geometry and this decision does
  not give it to them. New row.
- **The consequence found while writing the card, and it is the one that bites.**
  A transient pool is necessary and **not sufficient** to put changing text on the
  screen. Everything drawn is an entity, the draw system walks the mesh table
  (`3d/include/3d/mesh_component.h`), and **that table has no way to change a
  mesh's geometry** — `voe_3d_mesh_add` is the only writer and its comment says it
  is direct *because it is creation*. A per-frame block is not creation. So the
  changing-text path needs a second, much smaller decision about whether a mesh's
  geometry is replaceable and by what mechanism — direct call or intent and a
  system, under ADR-0011 and ADR-0017. **ADR-0085 takes it**; card 028 names it in
  `blocked-by`. This ADR is not reopened by it: the pool is right either way, and
  the omission is that this ADR reasoned entirely inside `render` and `text` and
  never looked at how the id reaches the table.

## Rejected options and why

**Option B — a quad per glyph, instanced.** It is the better shape for text at
scale and it is still the wrong move now. It fires the deferred scale bundle with
a label as the only evidence, when ADR-0081 deliberately bundled those three rows
one day earlier and gave them a trigger with real content behind it. It also
solves one of the three blocked consumers. If instancing later earns its place it
will be built over the transient pool, so nothing is lost by not taking it here.

**Option C — rewrite in place.** Not a real option once two frames in flight are
counted. Every version of it is either Option A with the bookkeeping moved to the
caller, or the stall it was meant to remove.

**An engine-side cache invalidated on change.** Rejected on the invalidation
failure mode rather than on performance: it has nowhere to live that does not drag
D-068 forward, and a stale cache draws yesterday's text. The cache worth having is
the caller's, in CPU memory, and needs nothing from us.

## Questions this opens

- **D-118 — persistent-mutable geometry**: a mesh too big to stream sensibly that
  changes rarely. Neither pool serves it. Trigger: the first such mesh — a terrain
  chunk, a deformed character, skinning.
- **D-119 — the transient pool's per-frame capacity, as a number**, and whether
  overrun is a returned refusal, a dropped draw, or something a program can ask
  about before it starts writing.
- **D-120 — what stops the transient pool being used for geometry that never
  changes.** It is the path that cannot fail at startup, which makes it the path of
  least resistance for exactly the wrong caller. Documentation, a debug count, or
  nothing.
