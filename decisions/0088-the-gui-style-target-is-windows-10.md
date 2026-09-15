# 0088. The GUI style target is Windows 10 — direction, and it keeps the GUI a module

- **Status:** Accepted
- **Date:** 2026-09-07
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Closes:** D-133

## Context

The principal, immediately after setting the theme direction:

> Lets aim for a windows 10 like gui style.

A style target is not usually an architecture decision. **This one is**, for two
reasons that only appear when it is checked against what is already decided: it
changes what the renderer is asked for, and it partly answers a question a spike
was just commissioned to settle.

What the target means concretely — Windows 10's shell, not Windows 11's:

- **Flat.** No gradients, no bevels, no material depth.
- **Square corners.** Rounding arrived with Windows 11.
- **Solid fills and hairline borders**, one pixel wide.
- **One accent colour**, chosen by the user and applied to selection, focus and
  active elements, over a light or dark base.
- **A neutral humanist sans**, generously spaced, low contrast.
- **Almost no shadow.** Flyouts get a faint one; nothing else does.

Constraints already fixed:

- **ADR-0087 — a theme is a few authored colours plus a derivation**, composing
  nearest-wins, derived in a perceptual space. Spike commissioned for the count.
- **ADR-0083 — a module produces things to draw; a switch changes how drawing
  happens.** A switch is core, ships off, and exists so nobody has to fork us.
- **ADR-0066 — the renderer stays as basic as it gets; cost is opt-in.**
- **ADR-0062 — Oxanium Regular is the default font, embedded**, one weight, one
  face, no fallback.
- **ADR-0078 — there is no anti-aliasing, and cut-out edges crawl.**
- **ADR-0049 / ADR-0074 — everything is in 3D space and in metres**, seen through
  a perspective camera. No screen space anywhere.
- **D-093** — `draw.slang` carries three uniform branches and the fourth triggers
  a rethink of variants versus specialisation.

## The finding that makes this worth an ADR

**Windows 10's look is the cheapest possible thing to ask this renderer for, and
Windows 11's would have forced a change to the core.**

A flat, square, solid-filled control is a quad with a colour. A hairline border is
a quad. Every one of those draws through the path that exists today — geometry,
a material, blended or not — with nothing new in `render` at all. The GUI stays
what ADR-0083 calls a module: it produces things to draw.

Contrast the alternative the principal did **not** pick. Windows 11 and Fluent's
acrylic are rounded corners plus translucency plus a background blur. A blur
changes what the frame is drawn into, which is ADR-0083's own definition of a
**switch** — core work, in the renderer, that every pipeline has to agree about.
Rounded corners want a signed-distance rectangle in the fragment stage, which is a
fourth uniform branch in `draw.slang` and therefore trips D-093 immediately.

> **Amended 2026-09-09.** ADR-0092 gave the GUI its own shader with a `kind` per
> element, so a rounded corner is no longer a `draw.slang` branch nor core work:
> it would be one more element kind, a module-side addition. Point 3 below still
> stands, on style alone — the target is square — and no kind is added before a
> widget needs it (D-168).

So this choice is worth more than taste: **it keeps the entire GUI outside the
core.** That is the roadmap shape ADR-0066 and ADR-0083 describe, arrived at by
picking a look rather than by an architectural argument, and it should be recorded
as a reason the target is good rather than only as a preference.

## The second finding: it lowers the risk on the theme spike

**Windows 10 is the best-known production example of exactly what ADR-0087
proposes.** Its entire theming model is *one accent colour, plus light or dark*,
with everything else derived — Windows generates a ramp of lighter and darker
accent shades and picks text colour on accent by contrast.

That is not four authored colours. It is closer to **two inputs and a derivation**,
which is *fewer* than the principal's opening guess and strong evidence that the
derived half of ADR-0087 works, because a billion desktops run it.

It does not close D-129 — Windows also carries semantic colours it does not derive,
which is the point the tech lead pushed back on and which this confirms rather than
settles. But the spike now has a known-good reference point to test against
instead of starting from nothing, and *"does our derivation reproduce something
Windows-10-like from an accent and a mode"* is a far sharper question than *"does
derivation work"*.

## Direction

1. **The GUI's visual target is Windows 10's shell**: flat, square, solid fills,
   hairline borders, one accent over a light or dark base, minimal shadow.
2. **This is a target, not a clone.** No Microsoft asset, icon, font or metric is
   copied. It names the shape and the restraint, not a pixel spec.
3. **The target is binding on what the GUI may ask the renderer for.** A control
   that needs a rounded corner, a gradient, a blur or a drop shadow is asking for
   core work, and under ADR-0083 that is a switch with its own decision — not
   something a widget card adds.
4. **Nothing is built until a card needs it.** Direction, in ADR-0073 and
   ADR-0077's sense.

## The one part that is genuinely hard, and it is not the colours

**A hairline border is a number of pixels, and nothing in this engine is natively a
number of pixels.**

Windows 10's crispness comes from a 2D compositor where one border is one physical
pixel, always. Here a panel is a quad in metres at an arbitrary distance and angle,
seen through a perspective camera, with no anti-aliasing (ADR-0078). A border
authored in metres gets thinner as you walk away and eventually disappears or
aliases into a crawling line; a border that stays one pixel needs a pixel-to-world
mapping that changes per panel, per frame — and is undefined for a panel seen edge
on, or in a headset where there are two eyes and no single pixel grid.

**Card 022 already named this as a known unknown** — *"nothing in this engine is
natively a number of pixels"* — and parked it. This ADR is the first thing to
depend on it, and the dependency is direct: hairlines are not a detail of the
Windows 10 look, they are most of what makes it read as crisp rather than cheap.

**This ADR does not solve it**, and deliberately: it is a real decision with real
options — borders in metres and accept the falloff, a per-panel pixel mapping, or
borders drawn as a distance field the way glyphs already are, which is the one
mechanism in this engine that is *already* sharp at any size. New row, and it is
the first thing a GUI card will hit.

## The collision worth flagging: the font

**Oxanium is not a Windows 10 typeface and nothing about it is close.** It is a
squared, technical display face; the target wants a neutral humanist sans, which is
most of why Windows 10 reads as calm.

**ADR-0062 is not reopened here.** It was decided on a different question — which
font must never be missing — and its reasoning is untouched by a style target.
Segoe UI is Microsoft's and cannot be shipped by us in any case, so "match the
font" was never available.

But the mismatch is real and a GUI built today would be a Windows 10 layout in a
games-console typeface. That is a decision for whoever writes the GUI cards, and it
has an existing home: **D-074, user-supplied fonts loaded at runtime**, which does
not exist yet and which a GUI wanting a different face would need. Recorded so the
GUI card meets it as a known constraint rather than discovering it when the first
button looks wrong.

## Blast radius

**Cheap.** A style target with no code behind it, taken before the cards it shapes
exist. Changing it later costs whatever has been built to it — and because point 3
keeps the demanding effects out of the core, what would have to change is a
module's worth of drawing rather than the renderer.

The expensive version of this decision is the one **not** taken: had the target
been Windows 11, the blur and the rounded rectangle would have gone into the core
early, and taking them back out later is the retrofit ADR-0016 exists to avoid.

Reversibility: **cheap.**

## Consequences

- **The GUI needs nothing new from `render`.** Quads, colours, blending, text —
  all built. That is unusual for a GUI and it is a direct result of the style.
  **Partly reversed by ADR-0092, 2026-09-07, and the distinction matters**: this is
  true of drawing the style *at all*, and false once *one draw call for the whole
  GUI* is the target, which needs an element buffer and a second pipeline. **The
  style still demands nothing new. The performance goal does.** Keep the two
  apart — this bullet has been read as *the GUI is free*, and what it says is that
  the look is free.
- **`draw.slang` keeps three uniform branches.** D-093 is not tripped.
- **The theme spike gains a reference implementation to test against**, and its
  question sharpens.
- **Hairline crispness becomes the GUI's first real technical problem**, ahead of
  layout, hit testing or widgets. New row, below.
- **The default typeface is off-target** and stays that way until somebody decides
  otherwise. See above.
- **The consequence I do not like:** a style target is the easiest kind of decision
  to quietly abandon. Nothing here is enforceable, and the first control that
  really wants a rounded corner will be argued for on its own merits by someone
  who has not read this. Point 3 is the guard and it is only writing.

## Rejected options and why

**Windows 11 / Fluent.** Not offered by the principal and worth recording as the
expensive alternative: acrylic is a blur, a blur changes what the frame is drawn
into, and that is core work by ADR-0083's own test.

**A style of our own.** Cheaper to argue about and much more expensive to finish.
A named target settles a hundred small questions without a meeting each, which is
most of its value.

**Deciding the hairline mechanism now.** It is a real decision with three viable
options and no card behind it. See below.

## Questions this opens

- **D-134 — how a hairline border stays crisp** on a quad in metres, at any
  distance and angle, with no anti-aliasing. Options: metres and accept the
  falloff; a per-panel pixel-to-world mapping; or a distance field, which is the
  one thing in this engine that is already sharp at any size and already has a
  shader branch. **The first thing a GUI card hits**, and it inherits card 022's
  known unknown.
- **D-135 — whether the GUI's typeface stays Oxanium.** Blocked on D-074, fonts
  loaded at runtime, which does not exist. Trigger: the first GUI card showing real
  controls.
- **D-136 — whether the accent colour is a program's choice, a user's, or the
  operating system's.** Windows 10 takes it from the OS, and reading it is
  platform-specific work in a folder that owns exactly that kind of thing. Trigger:
  the first themed control.
