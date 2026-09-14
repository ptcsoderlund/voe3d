# 041 — anchored children, padding on four sides, and the size that says something overflowed

status: review
claimed-by: claude-opus-5 (kanban-coder)
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

Four things, all inside `ui`, all small — and **they are one card on purpose.** Every
one of them lands in `layout.c`'s two passes and its one test file; splitting them
would put two coders in one file at the same time, which is worse than a card with
four numbered parts. If any part turns out bigger than it reads here, stop and report
rather than pushing on.


1. **A child can leave the flow and pin itself to its parent's edges**, recursively,
   mixed freely with rows and columns.
2. **A caller can read a node's measured size**, so that *this did not fit* is a
   comparison anybody can make. That is the prerequisite for a scroll area, and it
   is one accessor.
3. **One more distribution, `EVENLY`**, which is two lines and a test.
4. **Padding on four sides**, and **no margin, ever** — ADR-0095's second amendment of
   the day, and the one that changes an existing field.

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

## Scope — padding on four sides, and no margin

**Decided in ADR-0095's amendment, 2026-09-09**, from the principal's own proposal:
*"What if we skip margin and do padding only?"* Read that amendment; the reasoning
matters more than the field.

- **`voe_ui_container.pad` becomes four numbers, named by absolute side**: left, top,
  right, bottom. Millimetres, as now.
- **They are named absolutely and not by flow, and the header must say why**, because
  a reader will ask for `along_start`/`along_end`. Flow-relative padding flips meaning
  when a row becomes a column, which is the exact defect ADR-0095 removed by putting
  direction in the call; `pad_top` is always the top, in a row and a column alike, and
  it matches ADR-0099's surface with its origin at the top-left and Y running down.
- **There is no margin and there will not be one.** A child carries no outer spacing
  of its own, on any axis. **Write that in the header as a refusal with its reason**,
  not as a silence: with both, two sources of space meet between every pair of
  children and the system has to say whether they add or collapse — CSS collapses and
  it is the most-complained-about rule in layout. With padding only there is exactly
  one source of space between two children (`gap`) and one inside an edge (the
  padding), and nothing interacts.
- **Say the workaround in the same breath**: a single child needing space of its own is
  wrapped in a container with padding, which *is* that child's margin, and an unusual
  gap between one pair is a fixed-size box as a spacer. Somebody will want margin;
  the header is where they should find out why they do not have it and what to do
  instead.
- **Both passes change.** Measure adds left+right along a row and top+bottom across
  it — and the other way in a column — instead of twice one number; arrange insets the
  content box by each side separately. **The classic bug is counting padding twice**,
  once in each pass, and it is already in the hazards below; four numbers give it four
  ways to happen.
- **Anchors are measured against the content box** (ADR-0102), so an anchored child in
  a container with asymmetric padding sits inside the *padded* rectangle. Test that
  with a lopsided pad, because it is where the two halves of this card meet.

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

## Amendment 2026-09-10 — this card owns its call sites in `dev`, and that is not a scope escape

**Added by the tech lead in flight, on the coder's question**, which was the right
question and is answered here rather than in a reply nobody can read later:

> Card 041 changes `voe_ui_container.pad` from one number to four, which mechanically
> breaks two lines in `dev/src/interface.c` (`.pad = INTERFACE_INSET` and
> `.pad = 4.0f`). The card scopes itself to `ui`, and the coder rule says a
> sibling-folder edit is reported, not made — but without it `check.cmake` cannot go
> green and the card cannot move to `review/`. How should I handle it?

**Make the edit. The card's omission was mine, and three things say so.** **The general rule is now ADR-0113 and is written into `voe3d/CLAUDE.md` under *Work*, where it travels** — a card owns the call sites of a change it mandates, in any folder downstream of the one it names. You asked the question that produced it.

1. **`dev` is not a sibling of `ui`; it is downstream of it.** What the folder rules
   forbid is an *upward or sideways* dependency and reaching into a sibling's source
   directory. Nothing here adds a `DEPENDS`, reverses an arrow or reaches sideways —
   `dev` already depends on `ui` and goes on depending on it in exactly the same way.
   The dependency graph is untouched by this edit.
2. **This is not something you discovered on the way past; it is arithmetic the card
   ordered.** The rule's target is a repair *absorbed* into a card with nobody
   deciding — that is why it is written *seems to require* and *silent* scope escape.
   **Decided, not absorbed, is the distinction**, settled on 2026-09-09 when a coder
   read a fence written inside one card as a project law. Writing the edit into the
   card before it happens is precisely what removes the objection, and that is what
   this amendment does.
3. **The tree already expected it.** `dev/src/interface.c:126` carries a committed
   comment — *"The spacer card 041 removes. See the header."* — written by card 034.
   The project has been saying in its own source for days that this card edits that
   file. Only the card failed to say it.

And the alternative fails a rule that outranks the one you were honouring: a
repository that does not compile between two cards is a repository whose `check.cmake`
gate cannot be run at all.

### The exact scope in `dev`, and nothing beyond it

Three edits, all in `dev/src/interface.c`, and **no other file in `dev` and no other
change in this one**:

- **`.pad = INTERFACE_INSET` (line 125)** → the same 6 mm on all four sides. Same
  value, new spelling.
- **`.pad = 4.0f` (line 133)** → the same 4 mm on all four sides. Same value, new
  spelling.
- **The spacer box (lines 126–128) goes, and its job becomes `pad.top`** — the thing
  card 034's comment promised and the only real use of the new field in the tree.

**The arithmetic trap, because it is a subtraction nobody notices.** The spacer sits
*inside* the padding, so content currently starts at `INTERFACE_INSET + INTERFACE_TOP`
= 60 mm from the top, not at 54. **The new top pad is the sum, not `INTERFACE_TOP`.**
Collapse the two constants into one honest number rather than writing `6 + 54` at the
call site, and **say in your report which number you chose** — there is no test behind
this file and the principal is the only thing that will notice it being wrong.

**If a fourth thing in `dev` turns out to be broken, that one is reported and not
made.** This amendment names three edits; it does not open the folder.

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
  - **asymmetric padding on all four sides**: a row and a column, each with four
    different pad values, asserting both the children's rectangles and the
    container's own natural size — the natural size is where double-counting shows;
  - **an anchored child inside a lopsidedly padded parent**, landing inside the padded
    box and not the outer one;
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
- How you spelled the four pad values, and the header sentence refusing margin —
  quote it, because it is the sentence that has to hold the line for a year.
- Confirmation that the siblings-lay-out-as-though-absent claim is asserted, and
  what the arrange-order test looks like — 034 is written against both.
- **The three edits in `dev/src/interface.c` and the top-pad number you chose**, per
  the amendment — including confirmation that nothing else in `dev` moved.
- What you deliberately did not build: no scrollbar, no widget, no emission, no
  z-order beyond call order.

## Notes — as it landed, 2026-09-10

Verified on **Linux only** (WSL2, clang 21, lavapipe). `cmake -P check.cmake` exits
zero, every step: 13 folders standalone, 39 tests passing (`ui` 2 of them), analyser
clean over 106 files. **Windows was not checked** — there is no platform code in this
card and no shader, so what Windows could turn up is the `dev` interface's appearance
and nothing else; under ADR-0108 that would be a bug report, not a reopening.

### The spelling, and what was rejected

- **`voe_ui_container.anchor`, a field and not a second set of begin calls.** A zeroed
  `voe_ui_anchor` is a child in the flow, so nothing that does not ask for anchoring
  pays for it — and a *panel* is anchored by the very call that opens it, which is the
  principal's floating panel with no new surface at all. Rejected:
  `voe_ui_anchored_row_begin`/`_column_begin`/`_box`, which is three new calls for one
  idea and would have needed a fourth the day a widget wanted it.
- **The axes are `anchor.x` and `anchor.y`, absolute.** Rejected `along`/`across`: a
  child out of the flow has no flow to be relative to, and an anchor that changed
  which edge it meant when its parent turned from a row into a column is exactly the
  defect ADR-0095 removed by putting direction in the call — the same argument this
  card makes for absolute padding. Pinning to the top-right corner must not depend on
  the parent's direction.
- **The alignment is `voe_ui_across`, unchanged and untwinned**, per the card's
  "introduce no new vocabulary". The type name reads a little oddly on an axis called
  `x`; a second enum with the same four values in it would read worse, and that is the
  trade written into the header.
- **The offset is ONE number per axis, and a positive one always moves the child
  inward** — the gap from the near edge at START, from the far edge at END, a
  displacement in the positive direction at CENTER, and an inset off *both* edges at
  FILL. So the nine positions are symmetric and a negative offset is the way out of
  the parent. Rejected: two numbers for FILL and one for everything else, which is a
  wart on fifteen of the sixteen combinations.
- **A box cannot be anchored** — anchoring is a container's. A leaf that wants to float
  is wrapped in an anchored row, which is one node, and rule 10 says the second
  spelling waits until something calls for it.

### The answers the card asked for

- **`START`/`CENTER`/`END`/`FILL` mean the same four things anchored as in flow, with
  no exception anywhere.** The fixed-size-beats-`FILL` rule is the same rule and not a
  second answer: the child keeps the size it named and sits at the START of that axis,
  offset still applying. Asserted for the anchored path in `anchored_fill` beside the
  in-flow assertion that has been in `across` since 033, so the two cannot drift.
- **Over-constrained: the child's own declared size wins**, on both paths, because the
  more specific statement should win — a caller who wrote a number meant it and a FILL
  is a rule about everything.
- **A derived size that would be zero or negative is nought.** `FILL` with offsets
  that meet or cross clamps at nought and the corner still lands where the near offset
  says. A negative size reaching an element record is a rectangle wound the wrong way;
  an empty one is simply not seen. Asserted (20 mm parent, offsets of 15).
- **The accessor is `voe_math_float2 voe_ui_node_measured(const voe_ui_context *ui,
  voe_ui_node node)`.** It returns `content_natural` — what the measure pass computed
  the node's own *content* to be, which for a box is the content it declared and for a
  container is its children, gaps and padding. **It deliberately ignores the node's own
  FIXED or GROW declaration**, because that is what `voe_ui_node_rect` already reports
  and the whole value is the difference. So a grow child's measured size is what its
  content wanted, asserted at 25 against an arranged 100. Readable in the same window
  as `voe_ui_node_rect` and refused the same three ways. No `overflowed()` predicate.
- **`EVENLY` degenerates exactly as `SPREAD` does** — one child, or a run that already
  overflows, is START — and it is literally one shared guard in `arrange_children`
  (`flow > 1 && free_space > 0.0f`) rather than the rule written twice. The header
  sentence covering both is on `VOE_UI_ALONG_EVENLY`: *"SPREAD AND EVENLY DEGENERATE
  THE SAME WAY AND IT IS THE SAME SENTENCE: with one child, or with children that
  already overflow, there is no free space to distribute and both are START."* The
  enum also now says that the container's own `gap` still sits between children on top
  of the share, so "every gap identical" is true when `gap` is nought — which is why
  the test uses no gap and asserts four gaps of 17.5.
- **The four pad values are `voe_ui_pad { left, top, right, bottom }`**, that order:
  the two X sides then the two Y sides, near edge before far, so a positional
  initialiser reads in the same order as a rectangle's min and max. The refusal
  sentence in `layout.h`, quoted:

  > AND THERE IS NO MARGIN. A child carries no outer spacing of its own, on any axis,
  > and there will not be one. This is a refusal and not an omission: with both, two
  > sources of space meet between every pair of children and the system has to say
  > whether they add or collapse — CSS collapses them, and it is the
  > most-complained-about rule in layout. With padding only there is exactly one
  > source of space between two children, which is the container's `gap`, and one
  > inside an edge, which is its `pad`, and neither interacts with anything.

  The workaround follows it in the same breath: wrap the child in a padded container,
  and use a fixed-size box for an unusual gap between one pair.
- **The siblings-lay-out-as-though-absent claim is asserted** in
  `anchored_beside_the_flow`: box, anchored panel, box, in a row with a gap of 2 — the
  second box is at 12, which it could only be if the anchored one took neither space
  nor a gap.
- **The arrange-order test asserts the order itself**, not rectangles, because no
  rectangle can show it. `arrange_order` builds root / box A / anchored panel ONE
  holding a box / box B / anchored panel TWO, and asserts
  `voe_ui_paint_order` returns `0, 1, 4, 2, 3, 5` — root, A, B, ONE, ONE's child, TWO.
  So: parent before all its children, in-flow before anchored, anchored in call order.
  **This is the one test in the file that reaches into `src/`** (its own folder's
  `context.h`, not a sibling's), and the test header says why.

### What must not change — each one checked

- **The measure pass keeps its shape.** `measure()` is untouched; `measure_container`
  gained an anchored skip, an in-flow count and four-number padding, and the in-flow
  measurement path behaves identically. No in-flow child is measured differently.
- **Nothing about ADR-0095's six concepts moved.** `along`, `across`,
  natural/fixed/grow, `gap` and `pad` keep their meanings; `EVENLY` is a new value of
  an existing enum, added at the end so no existing number moved; anchoring sits
  beside the six rather than modifying one.
- **Deferred layout stays deferred.** Nothing is laid out during the calls, and
  nothing survives a frame — `subtree` and `paint` are per-frame like every other
  layout field, and `twice_over` still passes.
- **`ui` names nothing new.** `layout.c`'s includes are unchanged (`context.h`,
  `base/assert.h`, `stdio.h`). The tests still run with no window system and no
  graphics card.
- **Y still runs down and there is no negation in the folder.** Grepped: `layout.c`
  contains no unary minus. `END` on an anchor is `inner_min + inner_size - size -
  offset` — subtracting sizes along an axis, which `VOE_UI_ACROSS_END` already did
  since 033, and not a sign in front of a Y. An anchor to the bottom is a larger Y.

### The three edits in `dev`, per the amendment

Only `dev/src/interface.c` moved, and only these three:

1. `.pad = INTERFACE_INSET` → `{ INTERFACE_INSET, INTERFACE_TOP, INTERFACE_INSET,
   INTERFACE_INSET }` on the root.
2. `.pad = 4.0f` → `{ 4.0f, 4.0f, 4.0f, 4.0f }` on the hud panel. Same value.
3. **The spacer box is gone and its job is `pad.top`.**

**The top-pad number is 60**, and `INTERFACE_TOP` is now that. The trap the amendment
warned about is real: the spacer sat *inside* the 6 mm inset, so content began at
6 + 54 = 60, not 54. The two constants are collapsed into one honest number rather
than a sum at the call site, and the comment above it says so.

**The geometry is provably unchanged**, which is worth more here than a screenshot:
the root column has no `gap`, so the panel began at `pad(6) + spacer(54) + gap(0)` =
60 and now begins at `pad.top(60)`; X is 6 either way; the spacer emitted no element
record, so the element list is identical too.

**Nothing else in `dev` moved.**

### Reported and not made

- **`dev/src/interface.h` and `dev/dev.md` now describe a spacer that is gone.** The
  header says *"THE ROOT IS THE WHOLE SURFACE AND THE PANEL IS PUSHED DOWN THE PAGE
  WITH A SPACER... card 041 adds anchored children, and when it lands the spacer goes
  and the panel says where it wants to be"*, and `dev.md` says *"why its panel is
  pushed down the page with a spacer until card 041"*. Both are now false, and the
  first is doubly so: **the spacer went, but the panel does not anchor** — it is
  pushed down by its parent's top padding instead, which is what the amendment asked
  for. This is the amendment's "fourth thing", so it is reported and not made.
- **A suggestion, not done:** the hud panel is the obvious first customer for
  anchoring — `anchor.x = { END, 6 }`, `anchor.y = { START, 60 }` would let it say
  where it wants to be and would free the root of a top padding that exists only to
  dodge the readout. That is a change to what the principal sees and belongs on a card
  of its own.

### Deliberately not built

No scrollbar (that is 035), no widget, no emission, no element record, no z-order
beyond call order, no `AROUND`, no `overflowed()` predicate, no anchored *box*, and no
per-axis pair of offsets for `FILL`.
