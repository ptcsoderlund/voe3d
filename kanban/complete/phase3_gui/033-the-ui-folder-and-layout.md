# 033 — the `ui` folder, and layout

status: review
claimed-by: claude-opus-5 (kanban-coder)
blocked-by: -

> **AMENDED 2026-09-09, after this card was claimed. Read this first.**
> **The space this card lays out in has changed: it is Y DOWN from the TOP-LEFT
> corner, not Y up from the bottom-left.** The section *The space* below is
> rewritten and so are two of the tests; nothing else about the card moves.
> **Why:** `voe_render_element` — shipped, tested, and the only space these
> rectangles will ever be seen in — puts its origin at the top-left with y running
> downwards, *"which is what every interface in the world means by a coordinate"*.
> This card had said the opposite, following the engine's rule about which way up
> the **world** is. Both sentences described the panel's own millimetre space and
> only one can stand. Laying out in the space you emit into means the conversion is
> a copy; the other way costs a flip on every rectangle, another on every clip
> rectangle, and a flip **backwards** on the pointer that arrives for hit-testing.
> **The world is untouched** — a panel is still a thing standing in metres, +Y up,
> placed by a transform like anything else — and there is still exactly one Y flip
> in the engine, in the viewport. If you have already written the other convention,
> this is a sign change and a rename, not a redesign. Sorry for the churn; it is
> much cheaper now than after the emitting card.

Written by the tech lead under the standing grant. **Not a spin-off**: 033 is the
number the GUI plan reserved for this step, and it takes it. **It does not depend
on 030**, which is being worked now, nor on 031 or 032 — the plan had it above
them because it assumed this card also *emitted* what it laid out, and emission
is exactly the part that needs 030. Emission moves to 034. What is left is the
part with the design risk in it, and it stands alone.

The decisions behind this card are **ADR-0093** (the folder and its edges),
**ADR-0095** (the layout model and its vocabulary) and **ADR-0091** (immediate
mode; layout is two passes and sizes are right on the first frame). Everything you
need from them is restated here.

## Goal

A new folder `ui/` exists, configures standalone like every other folder, and can
**lay out nested rows and columns of boxes in millimetres**: given a tree of
containers and leaves built by calls between a frame's begin and end, it produces a
rectangle for every node.

**It lays out. It does not draw, does not read input, and does not know what a
widget is.** Its output this card is rectangles a test can assert. That is the
whole shape of it and it is why it is small.

## The one thing to understand before starting

**Immediate mode does not know how big a row is until the row's children have been
called.** That is the known weakness of the style, and it is why ImGui's windows
jump on their first frame. The decision behind this card rules that out: **layout
is two passes — measure, then arrange — and sizes are correct on the first frame.**

The only way to satisfy that inside an immediate API is to **defer**: the calls
between `frame_begin` and `frame_end` build a tree for this frame and nothing is
laid out until `frame_end`, which measures bottom-up and arranges top-down, once.
So:

- **Nothing about layout is kept between frames.** The tree lives for one frame in
  an arena the caller hands over, and is gone when the caller rewinds it. (The
  decision speaks of a keyed record kept between frames — that record is what
  *hit-testing and widget state* need, and those arrive on 034 and 035. Layout does
  not need it, and this card does not build it.)
- **A call cannot return a size**, because the size is not known yet. A `begin` or a
  leaf call returns a **node handle**, an index valid until the next `frame_begin`,
  and the rectangle is read through it after `frame_end`. That is what the test
  does, and it is what 034 will do to emit elements.

**No widget identity on this card.** Identity is the part of immediate mode that
reliably goes wrong, it is needed the moment something has state or a hover, and
nothing here has either. It is decided on 034, the first widget card, where the
first thing that needs a key exists.

## The vocabulary, and it is decided

From ADR-0095, which chose it precisely because naming in a public header is cheap
now and permanent later. Use these names.

- **Containers**: `voe_ui_row_begin`, `voe_ui_column_begin`, and one `voe_ui_end`
  for both. **Direction is in the call.** There is no direction setting.
- **`along`** — how children are placed along the flow: `START`, `CENTER`, `END`,
  `SPREAD`. `SPREAD` puts the free space *between* children, none at the ends, and
  is the only distribution offered.
- **`across`** — how a child sits across the flow: `START`, `CENTER`, `END`, `FILL`.
  Set on the container, applies to every child.
- **A child is one of three things along the flow**: **natural** (the default,
  measured from content), **fixed** (a size in millimetres), or **grow** (with a
  weight, sharing what is left in proportion). One number, never three — there is no
  shrink and no basis.
- **`gap` and `pad`**, in millimetres, on the container. One number each.
- **Overflow is not shrunk.** Content that does not fit sticks out past its
  container's rectangle; clipping is the element record's job when elements exist
  (030's clip rectangle), not layout's. Layout reports the true rectangles.

*Along a row* and *across a row* mean what they say, in a row and in a column
alike. Do not introduce `x`/`y`, `horizontal`/`vertical`, `main`/`cross` or any
flexbox name anywhere in the public header — that is the defect the decision
removed.

## The space

**Millimetres, two dimensions, X right, Y DOWN, origin at the panel's top-left
corner.** This is `voe_render_element`'s space exactly — read that type's header in
`render/include/render/device.h`, because it is the same surface described from the
other end, and matching it is the whole point: when card 034 emits, a laid-out
rectangle *is* an element's `bounds` with no arithmetic in between.

**A column lays its children from the top down and Y increases as it goes** — the
first child called has the smallest Y — so a column reads in call order, which is
what everyone expects, and the arithmetic is a running sum rather than a
subtraction. **There is still exactly one Y flip in this engine and it is in the
viewport** (ADR-0033, point 6); `voe_render_element_transform` is where the element
path's sign is reconciled with it. `ui` adds no negation anywhere, and if you find
yourself writing a minus sign in front of a Y, stop and report.

**The world's axes are unchanged and this is not a departure from them.** ADR-0033
fixes the axes of the world — +Y up, in metres — and a panel is a component on an
entity, standing in that world the right way up. What this card fixes is the *parameter space of a
flat surface*, which the world's handedness never described. Say that in one line in
the header, because a reader who knows the engine will expect Y-up and deserves to
find the reason where they look. **`text` is the one thing that keeps +y up**: a
glyph's box is in ems with +y up because that is what a font file says, and the one
place that gets turned round is wherever glyphs become elements, on 034 — not here,
and not in `text`.

Where this surface sits in the world is one matrix and it is not `ui`'s (ADR-0093).
Nothing here knows about pixels, metres, cameras or layers.

## Scope

### The folder

- `ui/CMakeLists.txt` — `voe_module(ui DEPENDS ...)`, on `text/CMakeLists.txt`'s
  model. Name only what this card uses; `math` and `base` will do. The *allowed*
  set is the map row below and is wider than what is linked today.
- `cmake/voe.cmake` gains a **`ui` row in `voe_allowed_deps`**: `render text math
  base`. ADR-0093 decided that row; this card makes it. Give the row a comment in
  the same voice as `text`'s and `sprite`'s, saying why `ui` is a leaf — it turns
  widget calls into elements and leaves *placing* them in the world to the caller,
  so it never names `platform`, `scene`, `ecs` or `3d`.
- `ui/ui.md` — the folder document, on `text/text.md`'s model: one paragraph per
  file saying what the file's own header explains.
- `ui/include/ui/layout.h` (or a name you argue for), `ui/src/layout.c`,
  `ui/tests/layout.c`.
- `CLAUDE.md`'s folder table gains the `ui` row. The tree drawing above it already
  lists `ui` as planned; make it real.

### The context

- A context created from an arena the caller owns and destroyed with it (rule 11).
  It holds capacities and nothing that outlives a frame.
- `frame_begin(context, frame_arena, root sizing)` and `frame_end(context)`. The
  root is a container like any other — a column, say — and **its size along each
  axis is either fixed or natural**, given at begin. Fixed is a panel of a known
  size whose children grow into it; natural is a panel that fits its children, which
  is the *stretch to children* the principal asked for and it costs nothing extra.
  Which axis gets which is the caller's, per frame.
- **Every node is pushed on the frame arena.** Arena pushes are not guaranteed
  adjacent (`base/arena.h` says so), so an array of nodes is a capacity chosen at
  context creation and pushed once per frame — the same shape as
  `voe_render_capacities` in `render/include/render/device.h`. **Overrun is a
  returned refusal** (ADR-0041), not fatal, naming the two numbers.
- Handles are indices into that array, valid until the next `frame_begin`. Rule 6:
  no double pointers, and indices make that easy.

### The two passes

- **Measure, bottom-up**: a leaf's natural size is what it declares; a container's
  natural size is its children's natural sizes summed along the flow plus gaps and
  padding, and the largest across, plus padding.
- **Arrange, top-down**: from the root's rectangle, each container gives its
  children rectangles — fixed children their size, grow children the leftover split
  by weight, natural children their measured size; then `along` places the run and
  `across` places each child in the cross direction, `FILL` stretching it.
- **Deferred to `frame_end`**, once, both passes. Nothing is laid out during the
  calls.
- Recursion or an explicit stack over the tree is **your call, say which**. Rule 14
  is about data read from a file and this tree is built by the program's own code,
  so recursion is legal here; the depth is the program's nesting and is bounded by
  what a person will type. The `truetype` composite walk chose an explicit stack
  and said why; follow or depart, with a sentence.

### The one question this card decides, out loud

**How a container's natural size is measured when it holds a `grow` child.** It is
circular — grow means *share what is left* and natural means *what the content
needs* — and every layout system answers it slightly differently. ADR-0095 left it
to this card on purpose so it is answered deliberately rather than discovered.

The options, and a recommendation:

- **A grow child contributes nothing along the flow** to its container's natural
  size, and its natural size across. **Recommended.** It is flexbox's own answer
  (`flex-basis: 0`), it keeps natural size well-defined without iteration, and it
  means a natural-sized panel with a grow spacer in it collapses the spacer to
  nothing, which is what a spacer should do when there is nothing to fill.
- **A grow child contributes its content's natural size**, so it never gets smaller
  than its content. Friendlier for a label that grows; costs a second measure and
  blurs what *grow* means.
- **Refuse the combination** — a container holding a grow child must itself be fixed
  or grow. Honest, and probably too strict for a settings panel.

Pick one, pin it with a test, and put the reasoning in the header. If you pick
against the recommendation, say what changed your mind.

### What a leaf is, this card

A leaf is **a box with a declared natural size in millimetres**, or fixed, or grow
along the flow. That is all: there is no text leaf, because measuring a string is
`text`'s and the label that needs it is 034. So this card **does not** add a
measure function to `text`. When 034 adds it, layout's *natural* is already the
hook.

## What must not change

State in your report that you checked each of these:

- **Nothing above `ui` is touched.** No `3d`, no `dev`, no `app`.
- **`render` and `text` are untouched.** `ui` may *link* neither yet.
- **`platform` is not named anywhere in `ui`**, including tests. `ui` is a leaf and
  the test is the proof: it must configure and run with no window system present.
- **No widget, no state, no identity, no hit-testing, no element emission.** Each of
  those is a later card and adding one here is the way this card fails.
- **The vocabulary is ADR-0095's.** No flexbox names in the public header.

## Where this card is likely to go wrong

- **Laying out during the calls** because it feels more immediate. Then a row's
  height is last frame's, and the first frame is wrong — the exact defect the
  decision rules out.
- **Keeping the tree between frames** to avoid rebuilding it. Rebuilding is free;
  the arena is rewound by the caller and the tree is a few hundred bytes.
- **`SPREAD` with one child, or with overflow.** One child sits at `START`; when the
  children already overflow there is no free space and `SPREAD` degenerates to
  `START`. Test both.
- **`FILL` across on a child that also declared a fixed across size.** Decide
  which wins — the container's `FILL` or the child's fixed — say so, and test it.
  The recommendation is that an explicit fixed size on the child wins, because the
  more specific statement should.
- **Padding counted twice**, once in measure and once in arrange. The classic
  off-by-`pad`.
- **Y, and the hazard is now the opposite of what it was.** The space runs down
  from the top-left, so a column accumulates and nothing is subtracted. The way to
  get this wrong is to *reintroduce* a flip — out of habit, or because the engine's
  world is Y-up — and the result would look right in a symmetrical test. Test that
  the first child of a column has the **smallest** Y, and lay out something
  deliberately asymmetric while you are there.
- **Naming things for the GUI's widgets** — `button_size`, `label_gap`. There are
  no widgets here.

## Verify

- `cmake -P check.cmake` exits zero, all steps, all tests, analyser clean. This is
  the first card to add a folder, so the map check, the standalone configure and
  the includes guard all have a new row to check; if `check.cmake` needs a change
  to see `ui`, that change is in scope and you say what it was.
- **Tests are plain C, one per module** (rule 12), and this wants a real set,
  asserting rectangles to a tolerance:
  - a row of three fixed boxes with `gap` and `pad`, each `along` value;
  - a column of the same, checking Y **increases** in call order — the first child
    called sits at the smallest Y, against the top edge;
  - grow children sharing leftover by weight, beside fixed and natural siblings;
  - `across` `START`/`CENTER`/`END`/`FILL`;
  - a natural root that fits its children (the *stretch to children* case);
  - the grow-in-natural-container answer you chose;
  - overflow: children larger than a fixed container keep their true sizes and
    stick out;
  - `SPREAD` with one child and with overflow;
  - nesting: a row inside a column inside a row, three deep, rectangles right;
  - capacity overrun refused, and the next frame lays out correctly;
  - two frames in a row on the same context give the same rectangles (nothing
    leaked between them).
- No screenshot — nothing draws.
- Windows is the principal's, though there is no platform code here and this
  should simply work.

## Report when this lands

- The public header's surface — every function and struct, briefly — so 034 can be
  written against what exists rather than what was planned.
- Whether the amendment at the top of this card cost you any rework, and what, so the
  next in-flight amendment is priced honestly.
- The grow-in-natural-container answer and why.
- `FILL` against a fixed across size: which won and why.
- Recursion or a stack, and why.
- The node capacity you chose as a default and what one node measures.
- Whether `check.cmake` needed to learn anything to see a new folder.
- What you deliberately did not build, so the next person knows layout is finished
  and the GUI is not.

## Notes (coder, 2026-09-09, Linux/WSL, claude-opus-5)

**Implemented in full. `cmake -P check.cmake` exits zero — every step, 37 tests,
analyser clean over 99 files.** The two new numbers are `ui/tests/layout.c` and
the two files it covers. Not in place, for the reason cards 029 and 030 already
record about this machine: no `ninja`, `slangc`, `wayland-scanner` or
`pkg-config`, no sudo, and a 9p mount CMake refuses to build inside. A private
toolchain was assembled in scratch (the ninja release binary, the Slang 2026.17
Linux release, and Ubuntu `.deb`s for pkgconf and libwayland extracted without
installing), the tree was mirrored onto ext4, and the unmodified script was run
on the mirror. Windows checked, and there is nothing platform-specific here to
check — no OS header, no window system, and `platform` is named nowhere in the
folder including its test.

Two things were proved rather than assumed, because a passing test proves nothing
on its own:

- **The test catches a wrong number.** Two expectations were perturbed by 1 and
  by 0.5 in the mirror; both came back named, with the line of the case.
- **The test catches a reintroduced Y flip**, which is the hazard the amendment
  names. A flip was put back into `arrange_children` in the mirror and eight
  checks failed, the asymmetric column among them.

### The report the card asks for

- **The public surface, all of it.** Nine types and eight functions in
  `ui/include/ui/layout.h`:
  `voe_ui_context` (opaque, long-lived, no destroy — it lives in the arena it was
  handed); `voe_ui_capacities { nodes }`; `voe_ui_rect { min, size }` —
  **`min` is the top-left corner, so a rect is `voe_render_element.bounds` with
  `min` as the `xy` and `size` as the `zw`, which is what 034 wants**;
  `voe_ui_size_kind { VOE_UI_SIZE_NATURAL, _FIXED, _GROW }`;
  `voe_ui_size { kind, value }` — one number, millimetres for FIXED, a weight for
  GROW, nought for NATURAL; `voe_ui_sizing { along, across }`;
  `voe_ui_along { START, CENTER, END, SPREAD }`;
  `voe_ui_across { START, CENTER, END, FILL }`;
  `voe_ui_container { size, along, across, gap, pad }` — `size` is the
  container's own, `along`/`across`/`gap`/`pad` are about its children;
  `voe_ui_node`, a `uint32_t` index, with `VOE_UI_NODE_NONE`.
  The calls: `voe_ui_context_new(arena, capacities)`,
  `voe_ui_frame_begin(ui, arena)`, `[[nodiscard]] voe_ui_frame_end(ui)`,
  `voe_ui_row_begin(ui, container)`, `voe_ui_column_begin(ui, container)`,
  `voe_ui_end(ui)`, `voe_ui_box(ui, content, sizing)`,
  `voe_ui_node_rect(ui, node)`.
  **Only `frame_end` is `[[nodiscard]]`, and the header says why**: the one thing
  that can fail is the frame, a refused node is reported once by `frame_end`, so
  a handle is not the failure channel — and making every handle nodiscard would
  put a `(void)` in front of every spacer and every container nobody needs the
  rectangle of, which is most of them.
- **The amendment cost about a fifth of the card, and no rework at all in the
  sense that matters.** It arrived after the header, the implementation and all
  eleven test cases were written and `check.cmake` was already green on the
  Y-up convention. What it actually cost: in `src/layout.c`, **two functions
  deleted** — `axis_start` and `axis_place` existed only to branch on which axis
  was Y, and with the origin at the top-left both axes are a running sum, so the
  branch is gone and the placement is one expression; two lines changed at the
  site; one paragraph of the file header rewritten. In the header, the *space*
  section and the rectangle's comment. In the test, **every Y expectation
  recomputed by hand — twenty-six numbers across the eleven cases — which was
  the bulk of the twenty-odd minutes**, plus the two cases the amendment asks
  for (the asymmetric column, and a negative corner from centring something
  bigger than its container). No signature moved, no name moved, no test was
  deleted, and nothing was redesigned. **The code came out simpler than it was**,
  which is worth pricing honestly too: the flip was the only place the two axes
  differed, so the amendment removed a branch instead of adding one. Cheap
  because it landed while the only reader of the header was me; the same
  amendment after 034 would have cost 034 as well.
- **Grow in a natural container: a grow child contributes nothing along the
  flow, and its content across.** The recommendation, taken. It keeps natural
  size well defined in a single pass, and it means a spacer in a panel that fits
  its children collapses, which is what a spacer should do when there is nothing
  to fill. Both halves are pinned by a test: the same tree in a natural row (24
  wide, spacer 0) and in a row of 40 (spacer 16). **Gaps are counted for every
  child, grow ones included** — the gap belongs to the run, not to a child — so
  the natural row is 24 and not 20, and that is tested rather than incidental.
- **`FILL` against a fixed size across: the child's fixed size wins**, and it
  sits at the START of the axis because there is no stretching left to do with
  it. The recommendation, and for the reason given: the more specific statement
  should win, and a caller who wrote a number meant it. Tested both ways round.
- **Neither recursion nor a stack, in either pass, and the tree's depth is not
  on the C stack at all.** The node array is in call order, which is a pre-order
  walk, so every child sits at a higher index than its parent: measure sweeps the
  array **backwards** and every descendant is already done; arrange sweeps
  **forwards** and every parent's rectangle is already set. Each node is touched
  twice, so both passes are linear, and no nesting limit is needed. The
  `truetype` composite walk had to choose because a font file's nesting is the
  file's; here the array the tree is built in already holds the order both passes
  want. The one stack in the file is the list of containers begun and not ended,
  which is the building phase and not a pass.
- **The node capacity, and what a node costs.** No default constant: nothing
  calls one, and rule 10 says a number nothing reads is not written. The header
  carries the guidance instead — a panel is tens of nodes, a whole interface
  hundreds, and the worked example asks for 256. **One node is 92 bytes** and the
  context is 48, so 256 nodes is about twenty-four kilobytes of the frame's arena
  per frame, pushed once at `frame_begin` alongside a same-sized stack of
  `uint32_t`. The header says that too, so the number is chosen once rather than
  economised on per panel.
- **`check.cmake` needed nothing — but the root `CMakeLists.txt` did, and that is
  worth a card of its own.** The script discovers folders by glob, so step 2
  configured `ui` standalone with no edit. The root build does not: it lists
  `add_subdirectory()` by hand, and until `ui` got its line **every step of
  `check.cmake` said `ok` while the folder's test was never built or run** — 36
  tests before, 37 after, and nothing in the output distinguished the two. That
  is a hole in the verification rather than a missing line in a file: a new
  folder can pass the whole script without a single one of its tests having
  executed. Making step 3 assert that every discovered folder is in the root
  build, or globbing the root, is a decision for the tech lead and not something
  to improvise in a card, so it is reported and not made.
- **`cmake/voe.cmake` gained the `ui` row**: `render text math base`, with a
  comment in `text`'s and `sprite`'s voice saying why `ui` is a leaf and why
  `platform`, `scene`, `ecs` and `3d` are not in the row and may not be added to
  it. The guard was seen to fire before the row existed — the first full run
  failed with `voe_module(ui): math is not an allowed dependency (ADR-0022)`.
  `ui/CMakeLists.txt` links `math base` only, which is what this card uses.

### What must not change — each one checked

- **Nothing above `ui` is touched** beyond the three files the card names or
  requires: `CLAUDE.md` (its table row and the tree line), `cmake/voe.cmake` (the
  map row) and the root `CMakeLists.txt` (the missing `add_subdirectory`). No
  `3d`, no `dev`, no `app`. Confirmed with `git diff --stat --ignore-cr-at-eol`,
  which lists those three and this card and nothing else.
- **`render` and `text` are untouched**, and `ui` links neither.
- **`platform` is named nowhere in `ui`**, tests included, and the test needs no
  window system and no graphics card.
- **No widget, no state, no identity, no hit-testing, no element emission.**
- **The vocabulary is ADR-0095's**, and no borrowed layout name appears in the
  public header — including the negative sentence that used to list them, which
  was removed so that grepping the header for one finds nothing.

### The one marker left behind

**`DEVIATION:`** in `ui/include/ui/layout.h`, above `voe_ui_frame_begin`. The
card sketches `frame_begin(context, frame_arena, root sizing)`; the root's sizing
is given on the root's own begin call instead, and `frame_begin` takes the arena
and nothing else. The card also decides that the direction is in the call and
that the root is a container like any other, and a `frame_begin` carrying the
root's size can honour neither: it would have to take a direction beside it,
which is the direction setting ADR-0095 removed, or leave the `size` field of the
first begin call meaning nothing at the top level. One call says how big the root
is and it is the call that opens it. Everything the card wanted from that
parameter still holds — the root is fixed or natural on each axis, grow is
refused there with a message, and which axis gets which is the caller's, per
frame. No `BLOCKED:` markers.

### What was deliberately not built, so the next person knows where the edge is

Layout is finished; the GUI is not. Not here, each by name: **no widget of any
kind**, no state, no keyed identity, no hit-testing, no pointer; **nothing
emitted** — no element record is written and `render` is not linked; **no text
measurement**, so there is no label and no measure function was added to `text` —
a box's `content` is the hook 034 fills in; **no clipping and no scrolling**,
because overflow is reported rather than shrunk and the clip rectangle is the
element record's; **no shrink, no basis, no wrapping, no per-edge padding, no
absolute placement, no z-order**, and no second distribution beyond `SPREAD`.
