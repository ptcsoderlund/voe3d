# 0095. Layout is a lighter flexbox, and its axes are named so they never swap

- **Status:** Accepted
- **Date:** 2026-09-07
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** in part, by ADR-0153 — *What overflow does* and the *No wrapping* consequence
- **Closes:** D-154

## Context

The principal, having settled the palette:

> Should we plan layout now? I would like us to have a lighter version of htmls
> flexbox. I think its great, but the naming is horrible. Too soon?

**Not too soon for the model; too soon for a card.** Layout is card 033 in the
stack, three cards above what is claimable, and nothing on the board touches it.
But this is a decision about *vocabulary in a public header*, and naming is the
thing that is cheap now and permanent later — the same reason ADR-0073, ADR-0086
and ADR-0087 were taken as direction ahead of their cards. So: direction, no API,
no card.

**And he is right on both halves.** Flexbox's model is genuinely good — a direction,
children that either take their natural size or share what is left, alignment on
two axes — and it is the model almost every layout system has converged on. Its
naming is genuinely bad, and the reason is specific rather than a matter of taste.

Constraints already fixed:

- **ADR-0091 — immediate mode**, hierarchy as nested `begin`/`end`, layout two-pass
  over a keyed record so sizes are right on the first frame.
- **ADR-0089 — authored in millimetres.** Every number here is millimetres.
- **ADR-0090 — the GUI takes a font**, and **nothing may hard-code a text
  measurement**: a label's natural size is measured from the font in hand.
- **ADR-0092 — the element record carries a clip rectangle**, which already exists
  and which overflow can use.
- **Rule 10 — implement on demand. No "in case".**

## What is actually wrong with flexbox's naming

Worth writing down, because it is the thing being fixed:

- **`justify-content` and `align-items` do not say which axis they mean, and which
  axis they mean *changes* with `flex-direction`.** That is the whole problem. Every
  person who has used flexbox has set the wrong one, and they did so because the
  names encode *main* and *cross*, which are relative terms nobody holds in their
  head.
- **`align-items`, `align-self`, `align-content`** are three near-identical names
  for three different things.
- **`flex: 1 1 0`** is a three-value shorthand whose members are rarely understood
  separately, so it is memorised as an incantation.
- **`space-between`, `space-around`, `space-evenly`** — three names whose
  differences are famously unmemorable.

## Decision

**A lighter flexbox with six concepts. Direction is in the call, and the axes are
named `along` and `across`.**

### Direction is in the function name

`voe_ui_row_begin` and `voe_ui_column_begin`. There is no direction *setting*,
which means **there is no setting whose meaning flips underneath the other
settings** — the naming problem is removed at its source rather than renamed around.

### `along` and `across`, which are plain English and never swap

- **`along`** — how children are placed along the flow. `START`, `CENTER`, `END`,
  `SPREAD`.
- **`across`** — how a child sits across the flow. `START`, `CENTER`, `END`, `FILL`.

*Along a row* and *across a row* mean what they say, in a row and in a column
alike. Nobody has to know which axis is "main".

**`SPREAD` is the only distribution offered**, replacing `space-between`,
`space-around` and `space-evenly`. It is the one people reach for; the other two can
be had with a `grow` spacer, and if a card genuinely wants them it can argue for
them then.

### A child is one of three things

- **Natural** — the default, measured from its content. For text that means measured
  from the font, per ADR-0090.
- **Fixed** — a size in millimetres.
- **Grow, with a weight** — shares what is left over, in proportion.

**One number, not three.** There is no `shrink` and no `basis`: shrink is rarely
used deliberately and is a large share of flexbox's confusion, and basis is what
*natural* and *fixed* already express.

### `gap` and `pad`, in millimetres

Both on the container. Nothing else.

### What overflow does

**Content that does not fit is clipped**, to the clip rectangle ADR-0092 already
put in the element record. Not shrunk — that is the `shrink` this model does not
have. A container that wants overflow to scroll instead is the scroll area on card
035, which is the same clip rectangle with an offset.

## Blast radius

**Moderate, and paid at the cheapest possible moment.** Every widget call in the
engine's history will be written against this vocabulary, and renaming it after an
editor exists means touching every call site anybody has written.

Which is the argument for taking it now rather than inside card 033: **it costs
nothing today and it is the single most-typed vocabulary the engine will ever
have.**

What stays cheap: adding to it. `WRAP`, the two other distributions, per-side
padding, a `shrink` — each is an addition that breaks nothing, and each should wait
for a card that wants it (rule 10).

Reversibility: **moderate now, load-bearing after card 034.**

## Consequences

- **Card 033 has its vocabulary** and can be written against it when 032 lands.
- **The model is small enough to hold in the head**: two container calls, two
  alignment settings with four values each, three ways to size a child, two spacing
  numbers. That is the *lighter* the principal asked for, and it is roughly a third
  of flexbox's surface.
- **Anyone who knows flexbox will be productive immediately**, because the model is
  the same one. The names differ where the names were the problem.
- **No wrapping.** A toolbar that runs out of room clips rather than folding to a
  second line. That is a real limitation and it is the first thing likely to be
  asked for; it is left out under rule 10 rather than guessed at.
- **The consequence I do not like:** `along` and `across` are a small dialect. A
  developer arriving from CSS has to learn two words, and every tutorial they have
  ever read says `justify-content`. The trade is that they learn two words once
  instead of getting them wrong for years, but it is a trade and not a free win.

## Rejected options and why

**Flexbox's own names.** Rejected by the principal, and the specific defect — names
whose meaning flips with the direction — is real rather than aesthetic.

**`main` and `cross`.** The honest technical names, and the source of the problem:
both are relative, so the reader must first work out which axis is main. `along`
and `across` are absolute relative to the flow, which is the property that matters.

**`x` and `y`, or `horizontal` and `vertical`.** Tempting and worse — they *are*
absolute, so in a column `horizontal` means the cross axis and every alignment
setting has to be rewritten when a row becomes a column. That is flexbox's bug made
louder.

**A constraint solver, or CSS grid's model.** Both more expressive, both far more
machinery, and neither is what was asked for. Grid earns its place for
two-dimensional layouts and a settings panel is not one.

**Waiting for card 033.** The alternative reading of rule 10, and rejected because
this is vocabulary rather than mechanism: nothing is being built, and a name is the
one thing that gets more expensive every day it is not settled.

## Questions this opens

- **D-155 — whether wrapping is added, and when.** The first toolbar that runs out
  of room. Trigger: that card.
- **D-156 — per-side padding.** One number today; a settings panel will probably
  want asymmetric padding somewhere and that is the moment to look. Trigger: card
  034 or 037.
- **D-157 — how a container's natural size is measured when it holds a `grow`
  child**, which is circular and every layout system answers slightly differently.
  It is a real detail and it belongs to card 033, but it should be answered
  deliberately rather than discovered. Trigger: card 033.


## Amendment · 2026-09-09 · a second distribution, and it is not space-around

**The principal:** *"true, we wanted flexbox spin-off. Start, center, end is probably
good. space between, space around."*

This ADR fixed the model at six concepts and said **`SPREAD` is the only
distribution**. `SPREAD` is space-between — the free space goes between the children
and none at the ends — so what is being asked for is a second one.

**Taken, as `EVENLY` rather than `AROUND`, and the substitution is deliberate.**

- **Space-around is the flexbox value people reliably get wrong.** Its end gaps are
  *half* the gaps between children, so a row of three does not look evenly spaced and
  nobody can say why from looking at it. It is a common source of *this layout is
  slightly off and I cannot see it*.
- **Space-evenly is what people mean when they reach for around**: every gap
  identical, the ends included. One sentence in a header explains it completely and
  there is no surprise left in it.
- **Exact CSS parity is not a goal of this ADR** — its whole content is that
  flexbox's *model* is good and its *naming and defaults* are not, which is why the
  axes do not swap and why `basis` and `shrink` were left out. Shipping the confusing
  member of a set for parity's sake would be the same mistake pointed the other way.

So `along` becomes `START`, `CENTER`, `END`, `SPREAD`, `EVENLY`. **`AROUND` remains
one arithmetic case away** if a caller ever specifically wants the half-gap
behaviour, and the day it is added it should be added with its trap written into the
comment.

**Degenerate cases follow `SPREAD`'s existing rule** and must be tested the same way:
with one child, or with children that already overflow, there is no free space to
distribute and the result is `START`. That sentence is already in the header for
`SPREAD` and it now covers two values.

Everything else in this ADR stands, including that this is the whole distribution
vocabulary: there is no `stretch` distribution, because stretching is what a `grow`
child does, and no per-child distribution override.


## Amendment · 2026-09-09 · padding on four sides, and no margin ever

**The principal:** *"What if we skip margin and do padding only? l,t,r,m?"* — the four
sides, read as left, top, right, bottom.

**Taken, and it is a better trade than adding margin: it deletes the problem instead
of answering it.** This ADR said *`gap` and `pad` in millimetres, one number each*, and
D-194 had been opened to ask what a per-child margin would do when it met `gap`.
**There is now no such question, permanently.**

### What changes

- **`pad` becomes four numbers, named by absolute side**: left, top, right, bottom.
- **There is no margin, and this is a decision rather than an omission.** A child does
  not carry outer spacing of its own, on any axis, ever.

### Why padding-only is sufficient

Everything a margin is reached for is expressible:

- **A single child needing space of its own** — wrap it in a container with padding.
  The wrapper's padding *is* that child's margin, and in an immediate API it is two
  extra calls at the call site.
- **Space inside one edge of a container** — that is what per-side padding is.
- **An unusual gap between one pair of children** — a fixed-size box as a spacer,
  which flexbox users do anyway.
- **A negative margin, to overlap** — not expressible, and not wanted. Overlap is what
  ADR-0102's anchored children are for, where it is deliberate and visible.

### Why it is worth refusing margin rather than allowing both

**With both, there are two sources of space between two children and the system has to
say whether they add or collapse.** CSS collapses, and it is the single
most-complained-about rule in layout; adding them surprises people the other way. **With
padding only there is exactly one source of space between two children — the `gap` —
and one source inside an edge — the padding.** Nothing interacts, nothing collapses,
and there is no rule to learn. That is this ADR's whole thesis applied to itself: take
flexbox's model, refuse the part that generates folklore.

The cost is real and is accepted: **a wrapper node where a margin would have been a
field.** It is more typing at a call site and one more node in a frame's tree, which
is an arena push measured in tens of bytes.

### Why the sides are named absolutely and not by flow

**A reader will ask why it is not `along_start`/`along_end`, so the header must
answer.** Flow-relative padding flips meaning when a row becomes a column — the exact
defect this ADR removed at source by putting direction in the call. `pad_top` is
always the top, in a row and in a column alike, and that matches ADR-0099's surface,
which has an absolute origin at the top-left with Y running down. **Absolute sides
never move underneath anybody.**

**Closes D-156** (per-side padding) and **closes D-194** (a per-child margin) — two
rows answered by one decision, in opposite directions.
