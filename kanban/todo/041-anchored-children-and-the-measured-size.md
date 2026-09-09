# 041 — anchored children, and the size that says something overflowed

status: todo
claimed-by: -
blocked-by: -

Written by the tech lead under the standing grant. **Not a spin-off**: it stands
beside 033 in the same folder and takes the next free number. **It needs nothing
unbuilt** — 033 landed and this extends it — so it is claimable now, in parallel
with 031, 032, 039 and 040. **No graphics card required**, like 033.

The decision behind this card is **ADR-0102** (a child may leave the flow and anchor
to its parent's edges), taken from the principal's own description of the first
interface he wants. **ADR-0095** still fixes everything else about the model and
**ADR-0091** still fixes the two passes. Everything you need is restated here.

## Goal

Three things, all inside `ui`, all small:

1. **A child can leave the flow and pin itself to its parent's edges**, recursively,
   mixed freely with rows and columns.
2. **A caller can read a node's measured size**, so that *this did not fit* is a
   comparison anybody can make. That is the prerequisite for a scroll area, and it
   is one accessor.
3. **One more distribution, `EVENLY`**, which is two lines and a test and is here
   because it is the same file.

**No widgets, no emission, no drawing, no scrollbar.** The scroll area is card 035
and it needs three things this card does not build. What this card does is make
overflow *knowable* and anchoring *expressible*.

## Why anchoring at all, since flow nearly does it

Say this back to yourself before you start, because it decides what you build:
**left-and-right snapping already exists** as `across: FILL`, top-and-bottom as a
grow child in a fixed parent, and right-alignment as a grow spacer. **What does not
exist is a child over its siblings** — a panel pinned to a corner, a badge, a
floating inspector — and that is the whole of what this card adds.

So: an anchored child is not a new kind of alignment. **It is a child that is not in
the row or the column at all.**

## Scope — the model, as ADR-0102 fixed it

- **An anchored child is out of the flow.** Its parent's row or column neither
  reserves space for it nor counts it in its own size.
- **Per axis, the alignment this folder already has, plus an offset in
  millimetres.** `voe_ui_across` — `START`, `CENTER`, `END`, `FILL` — which you
  implemented for in-flow children on card 033, means the same four things for an
  anchored child:
  - **`START` + offset** — pinned to the near edge, keeping its own size.
  - **`END` + offset** — pinned to the far edge, keeping its own size.
  - **`CENTER` + offset** — centred on the axis, keeping its own size.
  - **`FILL` + offset** — both edges, size derived from the parent less the offsets.
    **A child that declared a fixed size keeps it**, exactly as `VOE_UI_ACROSS_FILL`
    already promises in a row.

  **Four values on each axis is sixteen combinations, and that is the point**: it
  contains all nine of the corner, edge-middle and centre positions the principal
  asked for — *"topleft, topmiddle, topright, bottomright, bottommiddle, bottomleft
  and center"*, plus left-middle and right-middle which he did not list and which a
  side toolbar wants — and every stretch case, and the margins. **Introduce no new
  vocabulary for it.** A reader should learn `START`/`CENTER`/`END`/`FILL` once and
  find it means the same thing for a child in a row as for one pinned to a corner;
  ADR-0095 fixed the model at six concepts and this adds none.

  **Whether the two axes are named with the flow words or with something else is
  yours to decide** — an anchored child has no flow, so `along` and `across` may read
  oddly on it. Say what you chose and why; that is the one naming judgement on this
  card.
- **It contributes nothing to its parent's measured natural size**, exactly as a
  grow child contributes nothing along the flow — the answer 033 already took and
  argued. **State the consequence in the header and pin it with a test: a
  fit-to-children parent holding only anchored children has no natural size at
  all.** That is the trap CSS has; a sentence in the header turns it into a surprise
  once instead of a bug forever.
- **Anchors are measured against the parent's content box**, inside its padding. One
  sentence in the header, because this is the classic ambiguity in every system that
  has anchors and a reader will otherwise have to test it to find out.
- **Anchored children are arranged after their in-flow siblings, and among
  themselves in call order.** That order is what card 034 will emit in, and under
  ADR-0092 submission order is paint order — so **an anchored panel paints over its
  in-flow siblings**, which is exactly what a floating panel must do. Say it in the
  header; 034 depends on it.
- **Grow is meaningless for an anchored child.** There is no leftover to share.
  Asking for it is the caller's bug and asserts, on the model rule 13 and the rest of
  this folder already follow.
- **An anchored child is a container like any other** and may hold rows, columns and
  further anchored children, recursively. That is the request; test it three deep.
- **Nothing stops an anchored child from landing outside its parent's rectangle**,
  and that is deliberate: ADR-0095 has layout report true rectangles and leaves
  clipping to the element record. **Test that it does what it does** rather than
  leaving the behaviour undiscovered — a mistyped offset should draw a panel
  somewhere surprising, not be silently corrected.

**How it is spelled is yours**, on 033's and 038's precedent. Name the axes with the
words ADR-0095 chose where they apply, keep it out of flexbox vocabulary, and make
the header say plainly which of the two forms each axis is in. Say in your report
what you chose and what you rejected.

## Scope — one more distribution, `EVENLY`

**Small, and it is here because it is the same file and the same test suite.** Asked
for by the principal 2026-09-09 — *"space between, space around"* — and decided as
`EVENLY` rather than `AROUND` in ADR-0095's amendment, which carries the reasoning.

- **`voe_ui_along` gains `EVENLY`**: every gap identical, **including the ones at the
  ends**. `SPREAD`, which is already there, puts the free space between the children
  and none at the ends; this puts an equal share everywhere.
- **Add it at the end of the enum**, after `SPREAD`, so no existing value's number
  moves.
- **The degenerate rule is `SPREAD`'s and is already written**: with one child, or
  with children that already overflow, there is no free space to distribute and the
  result is `START`. Make the header's sentence cover both values rather than saying
  it twice.
- **Not `AROUND`.** Its end gaps are half its inner gaps, which is the flexbox value
  people reliably get wrong. If you find a reason it is needed, report it — do not
  add it on the way past.
- Test it beside the `SPREAD` cases: three fixed children in a fixed container,
  asserting **four** equal gaps rather than two; then one child, and overflow, both
  landing on `START`.

## Scope — the measured size

`voe_ui_node_rect` hands back where a node *came to sit*. The number that says
whether it fitted — **what the measure pass computed it wanted** — is internal.

- **Expose it.** One accessor beside `voe_ui_node_rect`, readable in the same window
  (after `frame_end`, until the next `frame_begin`), returning the node's measured
  natural size in millimetres.
- **Its whole purpose is the comparison**, so say that in the header: a container
  whose measured size along the flow exceeds its arranged size has content that did
  not fit, and *that* is what a scroll area will act on. Do not add an
  `overflowed()` predicate — the caller can subtract, and a predicate would have to
  choose a tolerance nobody has asked for yet.
- **A grow child's measured size is what its content wanted**, not what it was
  given. Make sure that is true and test it, because it is the case where the two
  numbers most obviously differ and it is the one a scroll area will hit.

## What must not change

State in your report that you checked each of these:

- **The measure pass keeps its shape.** Anchored children are skipped in it; nothing
  else about it moves. If you find yourself changing how in-flow children are
  measured, stop — this card is arrange-pass work plus one accessor.
- **Nothing about the six concepts of ADR-0095 changes.** `along`, `across`,
  natural/fixed/grow, gap, pad keep their meanings exactly. Anchoring sits beside
  them; it does not modify one of them, and `EVENLY` is a new value of an existing
  one rather than a new concept.
- **Deferred layout stays deferred.** Nothing is laid out during the calls, and
  nothing is kept between frames.
- **`ui` names nothing new.** No `render`, no `text`, no `platform`. The tests still
  run with no window system present.
- **Y still runs down from the top-left** (ADR-0099), and there is still no negation
  anywhere in this folder. An anchor to the *bottom* edge is a larger Y, not a sign.

## Where this card is likely to go wrong

- **Anchoring against the parent's border instead of its content box.** Both are
  defensible and only one is decided; the pad is *inside*, so the anchor is too.
- **Counting an anchored child in the parent's natural size.** Then a
  fit-to-children panel grows to contain a floating badge, which is the opposite of
  what a floating badge is for.
- **Arranging anchored children before their siblings**, which reverses the paint
  order and puts the floating panel underneath. It costs nothing to get right and it
  is invisible until 034 emits.
- **`FILL` plus a declared size on the same axis** is the same over-constraint the
  in-flow path already resolves, where the child's own size wins. **Keep it
  consistent rather than deciding it afresh**, and say in the header that it is the
  same rule — two paths answering one question differently is how a layout becomes
  folklore.
- **Zero or negative derived size** when both edges are pinned and the offsets
  exceed the parent. Decide what that is: nothing drawn is defensible, a negative
  size reaching an element record is not.
- **The measured size of a node whose content changed nothing** — do not cache it
  across frames. It is recomputed every frame like everything else here.

## Verify

- `cmake -P check.cmake` exits zero, all steps, all tests, analyser clean. The
  script now checks that a folder is in the root build; `ui` already is.
- Tests are plain C, no graphics card, in `033`'s own style — assert rectangles:
  - **the nine positions, as a matrix**: `START`/`CENTER`/`END` on each axis, with a
    non-zero offset, asserting the rectangle for each. That is the principal's own
    list and it is nine asserts of two numbers — cheap, and it is what somebody will
    read to learn the feature;
  - `FILL` on each axis and on both, which is the *fills its parent* case the
    principal's panel uses, with offsets acting as margins;
  - **`FILL` against a child that declared a fixed size**, matching what the in-flow
    case already does — the same rule, asserted for the anchored path so the two
    cannot drift;
  - an anchored child beside in-flow siblings: **the siblings lay out as though it
    were not there**, which is the load-bearing claim;
  - a fit-to-children parent holding only anchored children, whose natural size is
    nought — the trap, asserted rather than avoided;
  - anchored inside anchored inside a row, three deep;
  - an anchored child whose offsets put it outside its parent, landing where the
    arithmetic says;
  - arrange order: anchored children come after their in-flow siblings and in call
    order among themselves. **Assert the order, not just the rectangles** — it is a
    paint-order claim and 034 will rely on it;
  - the measured size against the arranged size for a container whose children
    overflow it, which is the comparison a scroll area will make;
  - a grow child's measured size being its content's want rather than its share.
- Windows is the principal's, and there is no platform code here.

## Report when this lands

- The spelling you chose for an anchor and for its two axes, and what you rejected.
- Confirmation that `START`/`CENTER`/`END`/`FILL` mean the same thing anchored as
  in-flow, including the fixed-size-beats-`FILL` rule — if they had to differ
  anywhere, say where and why, because that is a thing 034 and 036 will assume.
- Which loses when an axis is over-constrained, and why.
- What a zero or negative derived size does.
- The accessor's name and its exact contract.
- That `EVENLY`'s degenerate cases match `SPREAD`'s, and the header sentence that
  now covers both.
- Confirmation that the siblings-lay-out-as-though-absent claim is asserted, and
  what the arrange-order test looks like — 034 is written against both.
- What you deliberately did not build: no scrollbar, no widget, no emission, no
  z-order beyond call order.
