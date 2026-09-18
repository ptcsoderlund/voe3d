# 0089. The GUI is authored in millimetres, and a GUI unit is not a pixel

- **Status:** Accepted
- **Date:** 2026-09-07
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Closes:** D-134

## Context

ADR-0088 named the Windows 10 look and found that its hardest part is not colour
but the hairline: a one-pixel border is most of what makes it read as crisp, and
**nothing in this engine is natively a number of pixels** (card 022's known
unknown, inherited by D-134).

The principal proposed the answer:

> What if we say a gui pixel is one mm in our 3d space?

Constraints already fixed:

- **ADR-0033 — units are metres.** A millimetre is 0.001 of one; nothing new.
- **ADR-0049 / ADR-0074 — everything is in 3D space**, seen through a perspective
  camera. No screen space, no orthographic overlay.
- **ADR-0078 — no anti-aliasing.** A feature thinner than a pixel breaks up rather
  than fading.
- **Card 023 — both placements are first-class**: a panel on a wall you can walk
  behind, and a panel always in front of you.

## Decision

**A GUI is authored in millimetres.** One GUI unit is 1 mm in world space, so a
border is 1 mm, a button is around 32 mm tall, and a 800 × 600 panel is 80 × 60 cm
— slightly larger than a 27-inch monitor, which is the right instinct for something
you stand in front of.

**And the unit is not called a pixel.** This is the one place the tech lead pushed
back and it is worth the sentence:

> Calling it a pixel invites the reader to expect it to *be* a screen pixel, which
> is precisely the expectation that cannot be met and the one that caused this
> question. Calling it a millimetre makes a border a physical thing that behaves
> like a physical thing, and nobody is surprised when a physical hairline is hard
> to see from across the room.

The principal's number is adopted unchanged. Only its name is.

## What this solves, and what it honestly does not

**It solves the authoring problem completely.** Layout becomes integers again —
padding of 8, a border of 1, a row 32 high — which is what a GUI needs and what the
engine could not previously express. Every downstream question about spacing,
sizing and rhythm now has a unit to be answered in.

**It does not make a hairline crisp at every distance, and nothing can.** At some
distance one millimetre projects to exactly one screen pixel. Nearer, the border is
several pixels wide, which is correct and looks like a UI you have walked up to.
Further, it falls below a pixel and — with no anti-aliasing — breaks into a
crawling dotted line rather than fading.

**That is not a defect introduced by this decision. It is what a physical display
does.** A real monitor's one-pixel borders are also illegible from ten metres. The
value of this ADR is that it converts an unanswerable question — *how do we keep a
hairline crisp at any distance* — into an ordinary physical one: **at what viewing
distances is a GUI legible, and what happens outside them.**

The narrower question that remains is real and is a new row: a 1 mm line seen from
far enough away is sub-pixel, and this engine has nothing that handles minification
gracefully. The distance-field option ADR-0088 floated is still the interesting
one — not because it anti-aliases, which it must not, but because a field has a
defined value at any sample rate where a thin quad simply misses the sample grid.

## Consequences

- **Layout can be specified**, in cards and in themes, in numbers people can reason
  about. This unblocks most of the GUI's small decisions at once.
- **A HUD panel will usually want scaling.** At 1 mm per unit an 800-unit panel is
  80 cm, which is wider than the field of view at half a metre. That is a transform
  on the entity and not a second unit system — the placement is first-class, the
  authored size is the same, and only the scale differs.
- **A panel is now a physical object with a real size**, which is the mental model
  that makes both of card 023's placements behave the same way. That is a benefit
  the principal's framing produced and the pixel framing would have hidden.
- **The distance at which a GUI stops being legible becomes a thing programs must
  think about**, in a way it would not be on a flat screen. For a headset that is
  natural. For a HUD it is invisible because the distance is fixed.
- **The consequence I do not like:** 1 mm is a chosen constant with no measurement
  behind it, in an engine whose rule is that nothing is built that nothing measures.
  It is defensible — it is a *unit*, not a tuning parameter, and its whole value is
  being fixed and memorable — but the first GUI card should say whether it felt
  right at real viewing distances rather than assuming it.

## Rejected options and why

**Calling it a pixel.** Rejected on naming alone; the number is the principal's and
is adopted. A name that promises screen-pixel behaviour in an engine that cannot
deliver it is a support question forever.

**A per-panel pixel-to-world mapping** — computing, each frame, how many world
units make a screen pixel at that panel's distance, and sizing borders from it.
Rejected: it is undefined for a panel seen edge-on, it has two different answers in
a headset, it makes a panel's authored size change with the camera, and it
reintroduces screen space through the back door after ADR-0074 refused it twice.

**A smaller unit, such as 0.5 mm.** Finer control, less memorable, and it makes
default panels desk-sized rather than monitor-sized. Nothing argues for it yet; if
the first GUI card finds 1 mm coarse, that is a number to revisit and a cheap one.

## Questions this opens

- **D-137 — what a 1 mm line does when it falls below one screen pixel.** Options:
  accept the break-up as physical; a minimum screen-space width before the
  threshold, which is not anti-aliasing and must be argued as such against
  ADR-0078; or a distance field, which has a defined value at any sample rate.
  Trigger: the first GUI card, or the first person to walk away from a panel.
