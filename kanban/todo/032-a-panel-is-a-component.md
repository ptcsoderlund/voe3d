# 032 — a panel is a component

status: todo
claimed-by: -
blocked-by: -

Written by the tech lead under the standing grant. **Not a spin-off**: it follows
031 in the plan's table and takes its own number. **It does not depend on 031** —
a panel full of solid rectangles is a panel — and it does not depend on 033. It
can be worked beside either.

The decisions behind this card are **ADR-0093** (a panel reaches the frame the way
everything else drawn does, and `3d` never names `ui`), **ADR-0092** (the GUI is
one draw from an element buffer), **ADR-0104** (the GUI scales with the window and
`ui_scale` is the only calibration — the current word on this, after ADR-0100 and
ADR-0103), **ADR-0086** (layers are the developer's to
create and order),
**ADR-0085** (a component may hold an id that is rewritten every frame) and
**ADR-0098** (the loop owns the frame, and *build what changes this frame* is a
named phase inside it). Everything you need is restated here.

## Two corrections you need before you read ADR-0093

**First, the words.** ADR-0093 is titled *a panel reaches the frame as an entity*
and this card was called *a panel is an entity* until the principal corrected it on
2026-09-09. **An entity is an id and nothing else** — `voe_ecs_entity` is two
`uint32_t` and its own header says it *"names nothing but a row in whatever tables
hold one for it."* So a panel is **a component**: a row in a table this card adds,
keyed by an entity id, sitting beside the transform row that says where it is.
Nothing about the design changes; the phrase was hiding where the data lives, which
is exactly the thing a header has to get right. **Write it correctly in the header
you produce**, and if you find the old phrasing anywhere you touch, fix it there
too — the engine's older ADRs say *everything drawn is an entity* and they mean
*has a mesh component*.

**Second, the mechanism.** ADR-0093 describes the chain as **`ui` → an element
buffer id → a `3d` component → the draw system**. **There is no such id**, because card 030 built something
simpler and better: there is **one element buffer per frame slot**, `submit`
appends a record to the open frame's, and one draw draws them. Nothing is
allocated, nothing is named, and nothing survives the frame.

So a panel is not an id. **A panel is a range of this frame's elements, plus the
matrix that puts that range somewhere.** That is the amendment this card is
written on; the substance of ADR-0093 — panel as entity, `3d` never naming `ui`,
picking split outside the leaf, panels in the existing pass and layer order — is
untouched and is what this card builds.

## Goal

**Two panels on screen, drawn by the draw system**: one standing in the world, so
a cube in front of it hides it, and one in the overlay, so nothing does. Each is an
entity id carrying two rows — a transform and a panel — the same shape a drawn mesh
already has. Each is
one draw command. No `ui` folder is involved: the program writes element records
by hand.

**That is the checkpoint this card exists for.** After it, the engine can put an
interface on screen with no widget code at all, which is the honest place to be
before building widgets on top.

## Scope — `render`: a draw takes a range

- **`voe_render_frame_draw_elements` gains a first and a count.** Today it draws
  every element submitted, which is exactly one panel's worth and no more; two
  panels with two matrices need two draws over two ranges of the one buffer.
  It is called in two files — `dev/src/elements.c` once, and
  `render/tests/elements.c` about a dozen times — and changing all of them is in
  scope. Those tests mostly want *everything submitted*, so decide whether a range
  covering the whole frame is spelled out at each call or whether the test grows one
  helper, and say which.
- **A caller has to be able to learn its own range**, so add the count of what has
  been submitted to this frame: take it before submitting, take it after, and the
  difference is the range. **Name it so that it cannot be mistaken for
  `voe_render_frame_draw_count`**, which counts draw *commands* and is the number
  ADR-0092's claim is read off — two counts with similar names in one header is
  the kind of thing that is misread once and then repeated.
- **A range that runs past what was submitted is refused**, on `submit`'s model: a
  returned false and a line naming the numbers, not an assert. It is reachable from
  ordinary staleness — see the panel component below — and a refusal that costs one
  panel is better than a stop.
- **Where the surface's matrix comes from, and this is the part to get right.**
  `voe_render_element_transform(size)` maps millimetres to the whole target and
  **owns the element path's Y negation**; its own comment says a surface standing
  in the world *"is a different matrix and card 032's to build"*. A panel in the
  world needs the same millimetre step and then the world, the view and the
  projection.
  **The recommendation: `render` exposes the part that is its own** — element space
  in millimetres, Y down from the top-left, to the surface's own plane in metres, Y
  up — **and `3d` composes projection × view × model × that.** Then the sign lives
  in one folder and one function, ADR-0033 point 6 still holds, and
  `voe_render_element_transform` becomes the full-target composition of the same
  piece rather than a second answer. If you find a better split, argue it in the
  header; **what you may not do is write a second negation somewhere else.**
  A panel's size in metres is its size in millimetres, scaled — 1 mm is 1 mm
  (ADR-0089), so a 240 mm panel is 0.24 m before the entity's own transform says
  anything.

## Scope — `render`: the screen-filling surface stops stretching

**Added to this card on 2026-09-09**, after the principal looked at the exhibit in
a resized window: *"I noticed how current gui stuff stretches when window size
changes … I also want all gui elements to stretch by height and not width. So we
keep ratio on them."* **The source of that scale then changed twice in
one afternoon and landed back where it started** — read ADR-0104, ignore ADR-0100 and
ADR-0103, and note that **none of it changes what you build**: `render` takes
pixels-per-millimetre as a parameter and holds no policy, so the churn was entirely
about what the *caller* computes. It lands in this card because it is the same
function the card was already changing, and two cards editing one matrix is how a
sign gets lost.

**What is wrong today.** `voe_render_element_transform(size)` maps the authored
millimetre rectangle onto the whole target with `2/size.x` across and `-2/size.y`
down — **two independent scales.** A window whose aspect differs from the authored
rectangle's deforms everything in it, glyphs included, which makes the sharp-text
work of cards 025 and 027 wrong at every size.

**What it becomes.** **One uniform scale, and `render` is told what it is rather
than deciding it.** Add a call that takes **pixels per millimetre** and the target's
pixel size, and returns **the surface's millimetre size** — both axes derived by
dividing. That is the whole of it, and it is deliberately smaller than the version
this card carried this morning.

- **Why a parameter and not a policy**: where the number comes from is a fact about
  a display and about what a user chose, and `render` cannot know either. Same
  reasoning that keeps `ui` from reading input (ADR-0093). **Both modes anybody
  wants are then one multiplication at the call site**: a physical caller passes the
  display's pixels-per-millimetre, a proportional caller passes `target height ÷ the
  millimetres it wants to be tall`.
- **The caller needs the millimetre size back**, because it is what the layout root
  is laid out into and what the transform needs — so hand it back, do not bury it in
  a matrix. `voe_render_frame_begin` already takes a `voe_platform_size`, so the
  target's size is a type this folder names.
- **Keep `voe_render_element_transform(size)` as it is** — *map this millimetre
  rectangle onto the whole target* is an honest primitive and a world panel needs
  the same maths. What changes is that nobody computes `size` by assuming the
  window's aspect any more.
- **What the demo passes: the window's height divided by the surface's authored
  millimetre height, times `ui_scale`.** That is ADR-0104's whole formula and there is
  nothing else to it — no display is read, on any platform, and `platform` grows no
  surface for one. `ui_scale` is a plain number the program owns, default 1.0, and
  raising it makes everything bigger while showing less. Put it somewhere a person
  can change it and say in the header that it is the only calibration the engine
  has.
- **What the header says about millimetres, and get this right because the unit has
  had three meanings in one day**: on a screen-filling surface, an authored
  millimetre is **a proportion of the surface's authored height** — nothing physical,
  and nothing tied to a display. It becomes a physical size only through the window's
  size and whatever `ui_scale` the person at the screen chose. On a panel standing in
  the world it is still a real millimetre in metres, unchanged since ADR-0089. Two
  meanings, one name, and the header is where a reader will look for which is which.
- **No breakpoints, no form-factor logic, nothing per-platform.** A program that
  wants a different arrangement on a different shape of window writes that `if`
  itself. Nothing in `render`, `3d` or `ui` learns what kind of device it is on.

### And it snaps to the bottom-right, the way the readout already snaps to the top-left

**Asked for by the principal 2026-09-09**, after watching both things in a resized
window: the debug readout keeps its shape and stays in its corner, the exhibit
stretches. *"It should scale with height and snap to lower right (if snapping is a
thing)."*

**The pattern to copy is in the same program.** `top_left_of_the_view` in
`dev/src/main.c` places the readout by working out where the corner of the view *is*
at a fixed distance — `half_height` from the field of view, `half_width` from
`half_height * aspect`, then in from the edge by a margin. Its comment says the
consequence: *"so it stays in the corner when the window is resized."*

**Do the same thing in millimetres for the exhibit.** Once the surface's millimetre
size comes from the scale rather than from an assumed aspect, the exhibit's own
rectangles are placed **from the far edges inwards** — right edge minus its width
minus a margin, bottom edge minus its height minus a margin — instead of from the
constants `PANEL_X` and `PANEL_Y` it uses today.

- **This needs no anchoring feature and must not wait for one.** Card 041 adds
  anchored children to `ui`'s layout, which is what a real interface will use; the
  exhibit writes element records by hand, so for it snapping is a subtraction. Keep
  it that way — a `dev` program reaching into a layout feature it does not otherwise
  use would be the wrong shape of demonstration.
- **Say in the comment which corner it is glued to and why**, in the voice
  `elements.h` already uses about being a call site rather than a second engine.
- **The two behaviours together are the whole point**: it keeps its shape *and* it
  stays where it is put. Either one alone is half the fix, and the readout beside it
  is the reference — after this card, both things in the window should behave the
  same way when it is dragged about, which is the check to make before writing the
  report.

### The exhibit's own header is now wrong

`dev/src/elements.h` says: *"drag the window narrow and the rectangles stretch with
it, which is what a panel filling a window does and not a bug."* **Under ADR-0103
it is a bug**, and what replaces it is not *the same content scaled* but **the same
content at a fixed size with more or less room around it**. Fix the behaviour and fix the sentence, and say in the new one what
a narrow window now does instead: the same rectangles at the same shape, with fewer
millimetres of width to put them in. The exhibit is laid out at fixed millimetre
positions, so **some of it will fall outside a narrow window and be cut off** —
that is correct under this decision, it is what wrapping and scrolling will
eventually answer (D-155, card 035), and the header should say so rather than
leaving the next reader to think the exhibit is broken.

## Scope — `3d`: the panel component

- **`voe_3d_panel`**, a second drawable component beside `voe_3d_mesh` — the first
  time this folder has had more than meshes, which ADR-0093 said would happen. One
  table, one row per panel, registered like any other component. It holds:
  - **the element range for this frame** — first and count;
  - **the surface's size in millimetres**;
  - **a layer**, the same `voe_3d_layer` a mesh carries.
  It does **not** hold a matrix: the `voe_scene_transform` row under the same
  entity id is where a panel's placement lives, exactly as for a mesh, and the draw
  system already reads that table.
- **The size in millimetres is on the component and not implied by the elements.**
  That is D-150's second half, decided here: the surface's rectangle is what the
  millimetre-to-metre step needs, what hit-testing will need on 034, and what the
  elements' own bounds do not describe — forty rectangles do not say how big the
  paper is.
- **The range is a per-frame value and is stale by design.** The program rewrites it
  every frame in the phase ADR-0098 reserved for exactly this: after `_begin`, before
  the draw system walks. This is ADR-0085's precedent — a component holding an id
  that is replaced every frame — and it is the shape immediate mode requires.
- **What a panel that was not updated this frame does.** The buffer starts empty
  every frame, so last frame's range points at elements that are not there.
  **Recommendation: the draw system skips a panel whose range lies outside what has
  been submitted to this frame**, so *not updated* means *not drawn* rather than
  *drawn wrong*. Say in your report what you did, and say plainly what it does
  **not** catch: a stale range that happens to fall inside this frame's count draws
  somebody else's rectangles, and nothing short of a frame stamp on the component
  would catch that. Do not add the stamp on this card — record it and let the first
  program that trips over it argue for one.

## Scope — `3d`: the draw system walks two tables

This is the interesting half of the card.

- **A panel is blended, tests depth and writes none** — that is the element draw's
  state, and it is precisely `voe_render_frame_draw_blended`'s. So **a panel belongs
  in the blended group of its layer, sorted with the blended meshes**, not in a pass
  of its own. A see-through quad standing in front of a panel has to be able to
  come out in front of it, and the only thing that makes that true is one sort over
  both kinds.
- The machinery is already there and is nearly generic. `struct deferred` in
  `3d/src/draw_system.c` holds a geometry and an object record; `struct group` holds
  the deferred entries, their depths and the order the sort produces; `draw_group`
  sorts and issues. **What this card changes is that a deferred entry becomes one of
  two things** — a mesh draw or an element-range draw — and `draw_group` issues
  whichever it holds. `voe_3d_depth_sort` needs nothing: it already takes an array
  of view-space depths and a count, and knows nothing about what is being sorted.
- **The depth key for a panel is its origin's view-space depth**, the same key and
  the same function meshes use. One point per object is wrong for a long thing seen
  end-on, which the sort's own header already says is the trade being taken; a
  panel is a flat thing and the trade is no worse for it.
- **The groups are currently sized from the mesh count.** They become mesh count
  plus panel count. Overrun is not a thing here — the arena push is once per frame
  against a real count — but read `group_new` and keep its arithmetic honest.
- **Both layers, unchanged.** A panel in `VOE_3D_LAYER_WORLD` is occluded by what
  stands in front of it; a panel in `VOE_3D_LAYER_OVERLAY` is drawn after the depth
  clear and nothing in the world covers it. That is the heads-up case and it is not
  screen space: the panel keeps a real position in metres.

## Scope — the exhibit

`dev/src/elements.h` already says this card moves it: *"card 032 makes an element
surface an entity the draw system draws in layer order, and this call moves
there when it does."* Do that.

- **The exhibit becomes an entity with a panel component, standing in the
  world** — placed so that
  something in the scene passes in front of it during the orbit, because *a panel
  occluded by a cube* is the picture that proves the card.
- **And a second, smaller panel in the overlay**, which nothing hides. Two panels
  is what proves the range: one buffer, two draws, two matrices. A few rectangles
  is enough; it does not need to be a second exhibit.
- `main.c`'s printed draw count becomes the meshes plus one per panel, and the
  comment that reads the number says so.
- **Do not build a widget, and do not build a `ui` dependency.** The exhibit writes
  records by hand and that is the point of the checkpoint.

## What must not change

State in your report that you checked each of these:

- **`3d` does not name `ui`.** There is no `ui` in this card at all.
- **No new module edge anywhere.** `3d` already names `render` and `scene`.
- **The element record.** Not one field, not its size.
- **The mesh path**, the solid groups, the depth clear between the layers, and the
  order the four groups are issued in.
- **One Y negation in the engine** (ADR-0033, point 6). If you find yourself
  writing a minus sign in front of a Y anywhere but the one function that already
  owns it, stop and report.
- **`ui` is not created.** Card 033 does that and may already have.

## Where this card is likely to go wrong

- **Drawing panels after everything else** because it is easier. That is the
  shortcut ADR-0093 rejected by name: it hardcodes *the GUI draws last*, and a
  panel bolted to a wall is then visible through the wall. The whole card is about
  not doing this.
- **Sorting panels separately from blended meshes.** Two sorted lists issued one
  after the other is not a sort, and the failure only shows from certain angles —
  the exact shape of bug `depth_sort.h`'s header warns about.
- **The Y flip, twice or not at all.** A panel whose content is upside down is the
  cheap failure; a panel that looks right because two flips cancelled is the
  expensive one. `render/tests/elements.c`'s header explains why its arrangement is
  deliberately asymmetric — do the same for anything you draw on a panel here.
- **Millimetres and metres.** A panel 240 mm wide is 0.24 m wide in the world. Get
  that backwards and the panel is either a wall or invisible, and the number that
  tells you which is the one in the exhibit's header.
- **A push constant per panel that is not pushed before its own draw.** The element
  pipeline shares its layout with the mesh pipelines and both push at offset zero;
  two panels mean two pushes, each immediately before its own draw. The mesh drawn
  after them has to still be right — `render/tests/elements.c` already asserts that
  claim for one element draw, and now there are two.
- **Assuming the range is stable across frames.** It is not, and the component's
  header must say so in its first paragraph.

## Verify

- `cmake -P check.cmake` exits zero, all steps, all tests, analyser clean.
- **`render/tests/elements.c`** grows the range claims, in that file's style, by
  reading the picture back:
  - two ranges of the one buffer drawn with two different matrices, each landing
    where its own matrix says and **the frame holding exactly two draw commands**;
  - a range that runs past what was submitted, refused, with the frame otherwise
    intact and the elements that did fit still drawn;
  - a mesh drawn after two element draws, still right — the descriptor set and the
    push constant surviving both.
- **`3d`** gets the table and the ordering claims, without a graphics card where
  that is possible: a panel and a blended mesh interleaving correctly in the sort,
  a panel in each layer landing in the right group, and a panel whose range is
  stale being skipped.
- **A screenshot** with the world panel half-hidden by a cube and the overlay panel
  over everything, and the printed draw count beside it.
- **Two screenshots of the same overlay panel in two window shapes** — one near the
  authored aspect, one distinctly narrower — showing that **nothing is deformed and
  nothing changes size**, and that what changes is how much room there is. **Have the
  debug readout in both shots**: it already keeps its shape and holds its corner, so
  the two together are the before-and-after in one picture, and if the exhibit does
  not behave like the readout the card is not done. That is
  ADR-0103's whole claim and it cannot be asserted in a test; it is looked at.
- Tests that need no graphics card: the millimetre size comes out as the pixel size
  divided by the scale on both axes; halving the scale doubles both millimetre
  dimensions; a scale of nought is the caller's bug and is refused rather than
  producing an infinity that reaches a matrix; and **`ui_scale` at 2.0 halves the
  millimetres the surface holds**, which is the calibration knob asserted rather than
  eyeballed.
- Windows is the principal's.

## Report when this lands

- The component's fields, and the header sentence that says the range is one
  frame's.
- Where the millimetre-to-metre step and the Y sign ended up living, and whether
  `voe_render_element_transform` became a composition of it.
- How the sort came to walk two tables, and what `struct deferred` looks like now.
- What a stale range does, and what that does not catch.
- The draw counts, before and after.
- What the surface call looks like, where `ui_scale` lives in `dev` and how a person
  changes it, and the header sentence you wrote about what a millimetre is on a
  screen-filling surface.
- What you deliberately did not build: no widgets, no `ui` edge, no picking, no
  frame stamp.
