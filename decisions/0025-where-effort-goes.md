# 0025. Performance is a rendering requirement; loading only has to work

**Rule:** Rendering is written for performance from the start. File parsing and
loading only has to be correct. Where the two compete for time, rendering wins.

Work is detail-oriented: robust, and understood in detail, before moving on.

**Status:** Accepted · 2026-08-28

**Why:** Principal's call, and it is a genuinely useful split because it tells a
card where to spend effort. Loading happens once, off the frame; a slow glTF
reader costs a second at startup. Rendering happens sixteen million times an
hour, and a decision made carelessly there is not recoverable by optimising
later — it is recoverable by rewriting.

It also protects ADR-0023 from itself. Writing our own parsers is a large amount
of work, and this rule says plainly: make them correct, do not make them clever.

**Consequence for the renderer:** spending extra time up front on batching and
draw-call reduction is explicitly sanctioned rather than treated as premature
optimisation. It is not an exception to ADR-0020 — it is ADR-0020 applied, since
draw-call count is the one rendering cost that is knowable in advance rather
than discovered by measurement.

**Recommendation on draw calls — reach for the right tool first.** The principal
raised atlas offsets in the shader, baking models and textures together. That
instinct is right and the modern answer is stronger than atlasing:

- **Bindless indexing is already decided** (ADR-0018). Because every draw can
  index a different texture without rebinding anything, *different textures no
  longer force separate draw calls*. Much of what a texture atlas classically
  bought is already bought.
- **GPU-driven indirect rendering is the larger win.** Per-object data —
  transform, material id, texture id — lives in one GPU buffer indexed by the
  draw's instance id; compute culls; one indirect draw covers thousands of
  objects. That collapses draw calls by orders of magnitude, not by a factor.
- **Atlasing still earns its place** for memory locality, for streaming, and for
  fonts and UI where thousands of tiny images genuinely want to be one. It is a
  later tool, applied where profiling asks for it.

So: build indirect and bindless first, atlas where measurement points. This
is a recommendation, not a decision — the renderer is not designed yet.

**Cost accepted:** "Detail-oriented, robust before moving on" is slower per
card, and it is a deliberate trade against breadth. It also sits in tension with
scaffolding placeholders, which are deliberately thin — the rule governs code
that does something, not the build plumbing standing in for it.
