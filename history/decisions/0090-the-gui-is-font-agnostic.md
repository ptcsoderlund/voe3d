# 0090. The GUI is font-agnostic — it takes a font, it does not choose one

- **Status:** Accepted
- **Date:** 2026-09-07
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Closes:** D-135

## Context

ADR-0088 flagged a mismatch: Oxanium is a squared technical display face and the
Windows 10 target wants a neutral humanist sans, so a GUI built today would be the
right layout in the wrong typeface. It was recorded as a collision and deferred
behind runtime font loading.

The principal answered it directly:

> GUI should be font agnostic, so devs can choose what they seem fit.

Constraints already fixed:

- **ADR-0062 — Oxanium Regular is the default font, embedded**, precisely so the
  default can never be missing. One weight, one face, no fallback (`text/font.h`).
- **D-074 — user-supplied fonts loaded at runtime** is deferred; it needs
  `platform` file I/O and an asset-path convention, neither decided.
- **ADR-0087 — a theme is a cascade of roles** applied to a control and its
  subtree.

## Decision

**The GUI takes a font handle. It never names a font, embeds one, or assumes
anything about which one it has.**

Three things follow, and the first is the reason this is cheap:

1. **It costs nothing now and blocks on nothing.** *Font-agnostic* is a decoupling,
   not a loading mechanism. The GUI's API takes a `voe_text_font *`; where that font
   came from is not the GUI's problem. A program can hand it the embedded default
   today and its own face the day loading exists. **D-074 is not promoted to a
   blocker** — which is the outcome that was not obvious before the principal's
   sentence.
2. **ADR-0062 is untouched and Oxanium remains the default.** It answered *which
   font can never be missing*, and a GUI that is handed no font gets that one. A
   default is not an assumption.
3. **The typeface becomes a theme role.** Under ADR-0087 a theme carries the face
   and its size alongside the colours, cascading to a control's subtree the same
   way. That is where a developer changes it, and it means changing the font is the
   same gesture as changing the accent.

## The constraint this puts on layout, which is the real content

**Nothing in the GUI may hard-code a text measurement.** Every size that depends on
text — a button's height, a label's box, a column's width, the baseline a row is
aligned on — is measured from the font in hand, at layout time.

This is easy to write and easy to violate, and violating it is invisible until
somebody supplies a different face: the layout then looks subtly wrong everywhere at
once, with no single thing to point at. It is the specific failure this ADR exists
to prevent, so it belongs in the first widget card in those words.

It also interacts with ADR-0089: authored sizes are in millimetres and are the
developer's, while text-derived sizes come from the font. A row is *at least* 32 mm
high **and** at least tall enough for its text. Both, not either.

## Consequences

- **The Windows 10 target becomes reachable.** A developer who wants that look
  supplies a neutral sans and gets it; nothing in the engine stands in the way.
- **D-074 stays deferred but becomes clearly wanted.** Nothing is blocked on it,
  and the first developer who wants their own face will ask for it. That is the
  trigger, and it is now a much more likely one to fire.
- **`text` still offers one weight and no fallback**, so a GUI cannot ask for bold.
  Windows 10's own shell is very nearly one weight throughout, so the target does
  not need it — but *font-agnostic* does not mean *font-featureful*, and a card
  should not read it that way.
- **A supplied font can be missing glyphs.** The default cannot; a loaded one can,
  and `text` draws the font's own missing-glyph box rather than looking elsewhere.
  D-075 already holds that question and this makes it more likely to matter.
- **The consequence I do not like:** every font produces different metrics, so a
  layout that looks right in Oxanium may be cramped or loose in something else. The
  measure-don't-assume rule above is the mitigation, and it is only a rule.

## Rejected options and why

**The GUI picks a font.** What ADR-0088 was drifting toward by default, and it is
the shape that makes a toolkit feel like somebody else's. Rejected by the principal
directly.

**Making runtime font loading a prerequisite.** The tempting reading of
*font-agnostic*, and wrong: it would have put a `platform` file-I/O card and an
asset-path decision in front of every GUI card, for no benefit until somebody
actually has a font to load.

**Changing the default away from Oxanium.** Not asked for, and it would reopen a
decision taken on a different question. If the engine's own dev programs should
look Windows-10-like, that is its own small decision later.

## Questions this opens

- **D-138 — whether the theme's font role carries a size, a scale, or both.** A
  face plus an absolute size in millimetres is the obvious shape; a scale factor
  composes better through the cascade. Trigger: the first themed control.
