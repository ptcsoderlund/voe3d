# 033 — the `ui` folder, and layout

status: todo
claimed-by: -
blocked-by: -

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

**Millimetres, two dimensions, X right, Y up, origin at the panel's bottom-left
corner.** Y-up is the engine's convention for `text`, `sprite` and `ui` alike
(ADR-0033 names them so), and there is exactly one Y flip in the engine, in the
viewport; `ui` does not add a second. **A column lays its children from the top
down** — the first child called has the greatest Y — so a column reads in call
order, which is what everyone expects; write that sentence in the header, because
Y-up plus top-down flow is the one place a reader will hesitate.

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
- **Y.** Top-down flow in a Y-up space. Get the arithmetic in one function with a
  comment and test that the first child of a column has the greatest Y.
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
  - a column of the same, checking Y decreases in call order;
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
- The grow-in-natural-container answer and why.
- `FILL` against a fixed across size: which won and why.
- Recursion or a stack, and why.
- The node capacity you chose as a default and what one node measures.
- Whether `check.cmake` needed to learn anything to see a new folder.
- What you deliberately did not build, so the next person knows layout is finished
  and the GUI is not.
