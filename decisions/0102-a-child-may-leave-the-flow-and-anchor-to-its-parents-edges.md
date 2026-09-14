# 0102. A child may leave the flow and anchor to its parent's edges

- **Status:** Accepted
- **Date:** 2026-09-09
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Closes:** D-183

## Context

The principal described the first interface he actually wants:

> I want to be able to display a panel with a semitransparent background. I want it
> to snap to window left,right,top,bottom so it follows window resize. It should be
> able to detect overflow and show scrollbars. Children should be able to be
> positioned with snap l,r,t,b and/or flexbox like column/rows. Recursively.

Read against the code, most of that list exists or is scheduled. **The one thing
with no answer is snapping**, and the word that makes it a decision rather than a
detail is *and/or … recursively*: he wants anchoring available beside flow at every
level, not only at the top.

**Some of what *snap* means already falls out of the flow model.** Left-and-right is
`across: FILL`; top-and-bottom is a grow child in a fixed-height parent;
right-alignment is a grow spacer before the child. **What does not fall out is a
child pinned to a corner, out of flow, sitting over its siblings** — which is what
snapping means in a tool interface, and it is what WinForms' `Anchor` and CSS's
`position: absolute` with insets both provide.

Constraints already fixed:

- **ADR-0095 fixed the layout model at six concepts** and D-154's reasoning was that
  naming in a public header is cheap now and permanent later. Adding a seventh is
  therefore a decision, not an implementation detail.
- **ADR-0091 — layout is deferred to `frame_end`**, measured bottom-up then arranged
  top-down, once, so sizes are right on the first frame.
- **Card 033 answered D-157 by making a grow child contribute nothing along the
  flow** to its container's natural size — flexbox's `flex-basis: 0`. That is the
  precedent this ADR leans on: *contributes nothing to measure* is already a notion
  the built code has.
- **ADR-0092 — paint order is submission order.** So *which children are drawn on
  top* is decided by the order things are emitted, which makes ordering a layout
  concern and not only an emission one.
- **Card 033 exposes only the arranged rectangle** (`voe_ui_node_rect`); the measured
  natural size is internal. Overflow detection needs it and that is a separate,
  smaller gap.

## Options considered

### Option A — anchored children at any level, out of flow

A child may leave its parent's row or column and pin itself to the parent's edges,
recursively, mixed freely with flow siblings.

### Option B — only the surface's own children may dock

Anchoring exists one level down from the surface: panels dock to the window's edges,
and everything inside a panel is pure flow. One positioning model per level.

### Option C — no anchoring; express it with flow

`FILL`, grow children and grow spacers cover the common cases and nothing is added
to the vocabulary.

## Decision

**Option A, and the principal took it.** The deciding factor is his own last word —
*recursively* — together with the fact that **it costs less than it looks**: the
two-pass design already has *contributes nothing to measure*, and an anchored child
is placed in the pass that already knows its parent's final rectangle. Nothing about
the measure pass changes.

### What this pins

1. **A child may declare itself anchored**, and an anchored child is **out of the
   flow**: its parent's row or column neither reserves space for it nor counts it.
2. **Per axis, an anchored child either pins one edge with an offset and keeps a
   size, or pins both opposite edges and derives its size** from them. That is
   `Anchor`'s semantics and CSS's, and it is the whole of the model.
3. **An anchored child contributes nothing to its parent's measured natural size**,
   on card 033's own answer for grow children. **The consequence must be stated and
   tested: a fit-to-children parent holding only anchored children has no natural
   size at all.** That is the trap CSS has, and it is a surprise once rather than a
   bug forever if the header says it.
4. **Anchors are measured against the parent's content box** — inside its padding.
   Padding is a parent's statement about its own inside, and an anchored child is
   inside. This is the classic ambiguity in every such system and it gets one
   sentence in the header.
5. **Anchored children are arranged after their in-flow siblings and emitted after
   them, so they paint over them.** Under ADR-0092 paint order is submission order,
   so this is layout's business as much as emission's. Among themselves, anchored
   children keep call order.
6. **Grow is meaningless for an anchored child** — there is no leftover to share —
   and asking for it is the caller's bug.
7. **An anchored child is a container like any other**: it may hold rows, columns
   and further anchored children, recursively, which is the request.
8. **How it is spelled is the card's**, on card 033's and card 038's precedent. The
   ADR fixes the model; the header fixes the words, and says out loud which of the
   two forms each axis is in.

## Blast radius

**The public vocabulary, which is the expensive kind — and cheap this week.** `ui`
has one file, one card of code, no callers outside the engine and no authored file
anywhere that names these concepts. After the widget cards, an anchoring model that
turned out wrong would mean rewriting every widget that uses it.

What stays cheap regardless: the measure pass, which this does not touch.

Reversibility: **cheap now, load-bearing once widgets anchor anything.**

## Consequences

- **Two positioning models can be mixed at any level**, so a reader of a layout has
  to know which children are in flow and which are not. Mitigated by anchoring being
  stated at the child's own call site rather than on the parent — you can see it
  where you read it.
- **Paint order is now a layout concern.** It was already emission's under ADR-0092;
  anchored children make the *tree* carry an ordering claim. Card 034 has to emit in
  the order layout arranged, not in tree order, and that sentence belongs in both
  headers.
- **No z-order beyond call order**, deliberately. Two overlapping anchored siblings
  are ordered by which was called first, and a real z-index is a separate decision
  with its own trigger (D-184).
- **The consequence I do not like:** an anchored child can be positioned entirely
  outside its parent's rectangle, and nothing stops it. That is honest — layout
  reports true rectangles under ADR-0095 and clipping is the element record's job —
  but it means a mistyped offset draws a panel somewhere surprising rather than
  being refused. A test that asserts it, so the behaviour is chosen rather than
  discovered.
- **It makes the principal's panel expressible**, which is the point: a
  semitransparent panel anchored to three window edges, holding nested rows and
  columns, is then a program rather than an engine change.

## Rejected options and why

**Option B — only top-level docking.** Genuinely simpler and it covers the panel he
described. Rejected because it does not cover what he asked for — a floating child
inside a panel, a badge over content — and because the simplification saves almost
nothing: the arrange-pass work is identical, and the only difference is a refusal at
every level but one.

**Option C — flow only.** It is what the engine has, it covers the ordinary cases,
and it cannot put a child over its siblings. The first dropdown, tooltip or floating
inspector would reopen it, and each of those is a thing this engine's own dev tools
will want.

## Questions this opens

- **D-184 — whether overlapping anchored children get an explicit order.** Call order
  is the answer today. Trigger: the first two anchored siblings whose order matters
  and cannot be arranged by moving the calls.
- **D-185 — whether an anchored child may escape its parent's clip rectangle.** A
  dropdown or a tooltip wants to; a scroll area's content must not. The clip
  rectangle is per element (ADR-0092) so both are expressible, and which one an
  anchored child gets by default is undecided. Trigger: the first dropdown or
  tooltip.


## Amendment · 2026-09-09 · the per-axis form is the alignment we already have

**The principal, confirming and elaborating:** *"We can do snap to topleft,
topmiddle, topright, bottomright, bottommiddle, bottomleft and center."*

**That list exposed a gap and closed it in the same breath.** Point 2 above gave an
anchored child two forms per axis: pin one edge with an offset and keep a size, or
pin both opposite edges and derive the size. **Neither expresses *centred*** — pinning
both edges with equal offsets stretches the child rather than centring it — and three
of the positions he names (top-middle, bottom-middle, centre) need exactly that.

**And the fix is not a third form. It is the vocabulary already in the header.**
`voe_ui_across` is `START`, `CENTER`, `END`, `FILL`, implemented and tested by card
033 for in-flow children. **An anchored child's form on each axis is that same
four-valued alignment, plus an offset in millimetres.** Point 2 is superseded by:

- **`START` + offset** — pinned to the near edge, keeping its own size. *(Was: pin one
  edge with an offset.)*
- **`END` + offset** — pinned to the far edge, keeping its own size.
- **`CENTER` + offset** — centred on the axis, keeping its own size. **The form the
  edge-pin model could not express**, and the one his list needs.
- **`FILL` + offset** — both edges, size derived from the parent less the offsets.
  *(Was: pin both opposite edges.)* A child that declared a fixed size across keeps
  it, exactly as `VOE_UI_ACROSS_FILL` already promises for in-flow children.

**This is a strict simplification and it is why the amendment is worth making rather
than patching around.** Four values on each axis is sixteen combinations: it contains
**all nine** of the corner, edge-middle and centre positions — including the two the
principal did not list, left-middle and right-middle, which are what a side toolbar
wants — plus every stretch case, plus the offsets the readout in `dev` already uses
for its margin. **And no new vocabulary enters the engine**: a reader learns
`START`/`CENTER`/`END`/`FILL` once and it means the same thing for a child in a row
as for a child pinned to a corner, which is what ADR-0095 was protecting when it
fixed the model at six concepts.

Everything else in this ADR stands: out of flow, contributes nothing to measure,
measured against the parent's content box, arranged and emitted after in-flow
siblings so it paints over them, and recursive.
