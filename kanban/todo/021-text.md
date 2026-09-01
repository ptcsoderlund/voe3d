# 021 — text

status: todo
claimed-by: -
blocked-by: 017

> **THIN AND PROVISIONAL.** Written far ahead of the code it builds on, at the
> principal's explicit request, so the whole backlog is visible and reorderable.
> **Expect to rewrite it.** The tech lead's stated concern, recorded once and
> dropped: a card written before its foundations exist is written on guesses, and a
> stale card is worse than none. Treat this as a scope sketch, not a brief.

## Goal

Text on screen. Written by us — no font library (ADR-0023).

## The shape is already fixed

**Everything is in 3D space** (ADR-0049). Text rasterises into a texture and is
drawn on geometry. There is no screen-space path to reach for, and a "just for
debug" one is exactly what ADR-0049 exists to prevent.

Two mechanisms are permitted and they are not the same. Pick one, say why:

- **A glyph atlas** — rasterise glyphs, upload once, draw quads sampling it. No
  render-to-texture pass. Almost certainly the right first answer.
- **A panel rendered offscreen** — draw into a target, map it onto a quad. Needed
  for real UI, overkill for text alone.

## The blocking question, and it is not this card's to answer

**Transparency is on the *later* capability list and text needs it.** Glyph quads
are alpha-blended by nature. This card cannot start until the principal decides
whether transparency moves into v1. Flagged in the register; report it rather than
quietly enabling blending.

## Rough scope

- A font file reader. TrueType glyph outlines, written by us.
- Rasterisation to an atlas, with the fill rule stated — non-zero winding is what
  TrueType means.
- Layout: advance widths and kerning at minimum. Not shaping, not bidirectional
  text, not vertical scripts. Say what is refused.
- Quads in 3D space, in world or camera-locked.

## Known unknowns, deliberately unanswered here

Hinting, subpixel positioning, signed-distance-field versus bitmap atlas, atlas
eviction. Each is a real decision and none can be argued well before the previous
cards exist.
