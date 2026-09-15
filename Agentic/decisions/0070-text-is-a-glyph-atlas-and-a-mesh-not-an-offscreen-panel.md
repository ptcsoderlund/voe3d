# 0070. Text is a glyph atlas and one mesh per text block, not an offscreen panel

- **Status:** Accepted
- **Date:** 2026-09-05
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

The old card 021 offered two mechanisms and told the coder to pick one and say
why: a **glyph atlas** — rasterise glyphs once, upload once, draw quads sampling
it — or a **panel rendered offscreen** — draw text into a target and map that
target onto a quad. It called the atlas *"almost certainly the right first
answer"* and left the choice open.

That was correct when the card was written far ahead of its foundations. It is no
longer, for two reasons: the choice decides whether the `text` folder depends on
render-to-texture, which is a module-shape question rather than an implementation
detail; and card 023 already assumes an answer — it says its widgets use *"the
panel mechanism from card 021"*, which does not exist unless this decision goes
the other way.

Constraints already fixed:

- **Everything is in 3D space** (ADR-0049): text rasterises into a texture and is
  drawn on geometry, in the world or locked to the camera. There is no
  screen-space path and there will not be one. Both options obey this; it does
  not separate them.
- **The frame is already an offscreen target** (ADR-0051), so the *mechanism* for
  a panel exists in principle. What does not exist is a second target, a second
  frame, or any notion of rendering something that is not the frame.
- **The default font is embedded and cannot be missing** (ADR-0062): Oxanium
  Regular, one static weight, `.ttf`, in `text/fonts/`, `#embed`ded.
- **One draw per mesh, in table order, from the CPU** (`3d/draw_system.h`), with
  blended geometry sorted per object and outside the pooled single-draw path
  (ADR-0061).
- **Implement on demand** (ADR-0034), and **no per-frame cost the program did not
  ask for** (ADR-0066).

## Options considered

### Option A — a glyph atlas, and one mesh per text block
Glyphs are rasterised once into one texture. A run of text becomes one mesh of
quads — four vertices per glyph, all sampling the same atlas — drawn as **one
object, in one draw**, with the atlas as its base-colour texture. No second
render target, no second frame, no render-to-texture. What it makes permanent is
the atlas as the place anti-aliasing lives, and the assumption that a text block
is rebuilt as geometry when its string changes.

### Option B — an offscreen panel
Text is drawn into its own target and the target is mapped onto a quad. It gives
a whole page of text for one textured quad, and it is the mechanism real UI wants
— a widget tree drawn once and re-used for many frames. It costs a second render
target, a second frame's worth of machinery, a size policy for the panel, and a
resolution question the moment the quad is seen at any distance other than the
one the panel was drawn for. And it still needs the glyph rasteriser underneath
it, so it is Option A plus a target rather than an alternative to it.

### Option C — one quad per glyph, as separate objects
The naive form: every glyph is its own entity with its own material and its own
draw. Trivially simple to write, and it makes the first line of text on screen
into hundreds of blended objects — hundreds of per-frame sort keys and hundreds
of draws for something the user thinks of as one label.

## Decision

**Option A.** The deciding factor: the offscreen panel is not an alternative to
the atlas but a layer on top of it, so choosing it means building both — and the
half it adds is the half card 023 needs and card 021b does not.

- **`text` produces a texture and a mesh, and draws through the ordinary path.**
  It needs no render-to-texture, no second target, and no new capability from
  `render` beyond what card 021a adds.
- **One text block is one object, one mesh, one material, one draw.** Not one per
  glyph. This is the load-bearing half of the decision: it is what keeps the
  first sentence on screen from turning into hundreds of blended objects, and it
  is what keeps the CPU-sort and indirect-draw questions (D-070, D-071) on the
  sprites card where ADR-0061 put them.
- **Anti-aliasing lives in the atlas, as coverage in the alpha channel**, with
  the colour channels white everywhere — transparent texels included, which is
  what keeps a filtered glyph edge clean (ADR-0069). There is no MSAA and no
  post-process anti-aliasing in this engine, so this is the only anti-aliasing
  that exists anywhere in it.
- **Text is built once and does not change.** `voe_render_geometry_create` waits
  for the GPU to go idle and appends to a pool that has no destroy — its own
  header says streaming geometry in while drawing is *"a different mechanism and
  a different card"*. So a string that changes needs machinery that does not
  exist, and this decision does not invent it: on card 021b a text block is built
  at startup and stays. See D-092, which this makes concrete rather than
  hypothetical.
- **The offscreen panel stays unbuilt and stays the right answer for UI.** Card
  023 keeps it, as its own mechanism on its own card, where the widget tree that
  justifies it also lives.

**Not decided here:** the atlas's internals — bitmap versus signed distance
field, its size, its packing, whether it ever evicts. Card 021b takes the first
answers and D-091 carries the one that will come back.

## Blast radius

**Moderate, and concentrated in one promise.** The atlas itself is cheap to
replace — it is a texture and a table of glyph rectangles behind one folder's
interface.

What is expensive is **"a text block is one object"**. Every later thing that
touches text assumes it: the sort key is one point for a whole label, hit-testing
a character will need the mesh rather than the scene, and an editor's text
cursor will be a position within a mesh rather than a separate entity. Reversing
it means every text block becomes many objects and the sort cost the sprites card
was going to reason about arrives early and unbudgeted.

## Consequences

- **`text` depends on `render` for a texture and on nothing for a target**, which
  keeps it the same shape as `3d` and keeps the module map's promise that `text`,
  `sprite` and `ui` sit beside `3d`, each on `render`.
- **The statistics readout can be drawn with no asset pipeline and no file I/O**,
  because the font is embedded and the atlas is built at startup.
- **Text at a steep angle or a large scale will show the atlas's pixels.** That is
  the accepted cost of a bitmap atlas and it is what D-091 exists for.
- **A long string is a large mesh**, four vertices per glyph. A page of text is
  thousands of vertices, which is nothing for the geometry pools and worth
  knowing before someone is surprised by it.
- **The statistics readout stays on the console after card 021b.** It is the most
  obvious consumer of text and the one this cannot serve, because it changes
  every frame. Saying so on the card is better than a coder discovering it after
  writing a TrueType reader.
- **Card 023's description of its own mechanism is now correct rather than
  assumed** — it builds the panel, and it does not inherit one.
- **Nothing here costs a frame in a program that draws no text**, which is what
  ADR-0066 asks of every feature: the atlas is built when a font is first asked
  for, not at device creation.

## Rejected options and why

- **Option B — the offscreen panel.** Not rejected as a technique; it is adopted
  on card 023 for the case that needs it. Rejected as the *first* answer, because
  it is strictly the atlas plus a target, and buying the target on the text card
  means paying for the widget tree's mechanism before a widget tree exists
  (ADR-0034, ADR-0008).
- **Option C — a quad per glyph as separate objects.** Rejected on cost that
  arrives immediately and compounds: it turns one label into hundreds of blended
  objects, which is the exact per-frame cost ADR-0025 says to think about in
  advance and ADR-0061 deliberately pushed onto the sprites card.

## Questions this opens

- **D-091** — whether the glyph atlas becomes a signed distance field. Card 021b
  takes a fixed-size bitmap atlas because its first consumer is a camera-locked
  statistics readout at one size; the everything-is-in-3D rule (ADR-0049) makes
  arbitrary scale and oblique viewing far more likely here than in an engine with
  a screen-space path, so this will come back. Trigger: the first card that wants
  text in the world at a scale the atlas was not built for.
- **D-092** — **how text that changes reaches the screen at all.** Not a
  refinement: with `voe_render_geometry_create` a startup operation over a pool
  with no destroy, rebuilding a mesh per frame would stall the GPU and leak pool
  space until it ran out. The candidates are a streaming geometry path, or one
  unit quad drawn per glyph from a per-frame buffer — which is instancing, still
  on the *later* capability list. **The first consumer this blocks is the
  statistics readout**, which is the thing most wanted on screen and the reason
  card 020 left it on the console. Trigger: card 021b landing, since static text
  is where it becomes the obvious next ask.
