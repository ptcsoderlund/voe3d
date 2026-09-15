# 032 — a panel is a component

status: review
claimed-by: claude-opus-5 (kanban-coder)
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

## Amended 2026-09-09: two surfaces, and which scope is about which

**The card's coder found that the exhibit scope and the screen-filling scope
contradict each other, and he was right.** The exhibit scope was written when the
demo had exactly one element surface; the screen-filling scope was appended hours
later and inherited that surface's header without noticing the exhibit was moving
out from under it. Read the two together and the demo both does and does not fill
the window.

**The answer is that the demo ends up with three element draws, on two different
kinds of surface, and only one of them is what the screen-filling scope is about.**

| # | Surface | Reaches the frame by | What a resize does to it |
|---|---|---|---|
| 1 | **World panel** — the exhibit's 40 rectangles and 40 glyphs | entity + panel component, `VOE_3D_LAYER_WORLD`, sorted with blended meshes | nothing. It is a thing in metres and the window only changes the camera's aspect |
| 2 | **Overlay panel** — a few rectangles | entity + panel component, `VOE_3D_LAYER_OVERLAY` | nothing, for the same reason (ADR-0074: overlay is still in perspective, still in metres) |
| 3 | **Screen-filling surface** — new, small, no glyphs | `voe_render_frame_draw_elements` called directly with `voe_render_element_transform`, outside the draw system | **everything. This is the only surface the screen-filling scope describes.** |

**Two kinds of surface is not new and this card does not decide it** — it is what
ADR-0093 and ADR-0104 already say standing side by side. A world panel is an object
at a position in metres, where an authored millimetre is a real millimetre
(ADR-0089). A screen-filling surface has no position, is mapped straight onto the
target by `voe_render_element_transform`, and is where an authored millimetre means
*a proportion of the surface's authored height* (ADR-0104 point 5). **Both meanings
of the unit are live in this one card, on three surfaces, and the headers you write
are where a reader finds out which is which.** That was already asked for; this
table is what it is asking about.

**Surface 3 is not "drawing the GUI last", which ADR-0093 rejects.** That rejection
is about *world and overlay content* — a panel bolted to a wall must be able to go
behind the wall. A screen-filling surface is not in the world, has nothing to sort
against, and cannot be occluded by definition. Keeping it outside the draw system is
correct, and it is why the card tells you to keep
`voe_render_element_transform(size)` exactly as it is.

### What this changes in the text below

- **The exhibit still moves to the world panel**, as `dev/src/elements.h` already
  promises and as the exhibit scope says. Unchanged.
- **Surface 3 needs a little content of its own**, because the content that used to
  demonstrate the resize behaviour is leaving with the exhibit. **A handful of
  rectangles, no glyphs, no font, no second exhibit.** The property that matters, and
  the only one: **laid out at fixed millimetre positions measured from the surface's
  top-left, reaching far enough right that a narrow window cuts the far ones off.**
  Ticks along two edges plus a marked square is enough. How you spell it is yours.
- **`ui_scale` lives beside surface 3** and calibrates only it. It has nothing to say
  about a panel standing in the world, and the header should say so where a reader
  will otherwise assume one knob moves everything.
- **The corrected sentence about "fewer millimetres of width" and content "cut off"
  belongs to surface 3's header, not to `dev/src/elements.h`.** In `elements.h` the
  stretching paragraph is **deleted, not rewritten** — the exhibit is not on a
  stretched surface any more, so there is nothing there to correct.

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
- **What the demo passes — for surface 3, and only for it: the window's height
  divided by the surface's authored
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

### The stretching sentence, and where its replacement goes

**Corrected by the 2026-09-09 amendment above — read that table first, because this
paragraph originally addressed the wrong file.**

`dev/src/elements.h` says: *"drag the window narrow and the rectangles stretch with
it, which is what a panel filling a window does and not a bug."* **That sentence is
deleted rather than rewritten**, because the exhibit it describes is moving onto a
world panel where a resize does nothing at all. Say there instead what the exhibit
now is: a surface of a fixed size in metres, standing in the world, whose
millimetres are real millimetres.

**What replaces it belongs to surface 3's header**, and it is not *the same content
scaled* but **the same content at a fixed size with more or less room around it**.
Say what a narrow window does: the same rectangles at the same shape, with fewer
millimetres of width to put them in. Surface 3 is laid out at fixed millimetre
positions, so **some of it will fall outside a narrow window and be cut off** —
that is correct under this decision, it is what wrapping and scrolling will
eventually answer (D-155, card 035), and the header should say so rather than
leaving the next reader to think it is broken.

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
- **And surface 3, which stays where it is**: still submitted and drawn by a direct
  `voe_render_frame_draw_elements` after the draw system has walked, still using
  `voe_render_element_transform` — with its size now coming from the new call and a
  little content of its own, per the amendment at the top of this card. It is the
  one surface in the demo that fills the window, and the only one `ui_scale` moves.
- `main.c`'s printed draw count becomes the meshes, **plus one per panel, plus one
  for surface 3** — meshes plus three — and the comment that reads the number says
  which is which. **Two of those three go through the sort and one deliberately does
  not**, which is the sentence that stops the next reader thinking surface 3 was
  forgotten.
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
- **Two screenshots of surface 3 in two window shapes** — one near the authored
  aspect, one distinctly narrower — showing that **nothing is deformed and
  nothing changes size**, and that what changes is how much room there is, with the
  far content cut off in the narrow one. That is ADR-0104's whole claim and it cannot
  be asserted in a test; it is looked at. **Corrected by the 2026-09-09 amendment:
  this bullet named the overlay panel, which is the wrong object** — an overlay panel
  is in metres and in perspective, so it would have shown nothing either way.
- **The world panel in those same two shapes, unchanged**, which is the other half of
  the claim: the surface that is a thing in the world does not answer to `ui_scale`
  or to the window at all.
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

## Notes — claude-opus-5 (kanban-coder), 2026-09-09

### The card's own defect, and what came of it

The exhibit scope and the screen-filling scope contradicted each other and the
card was amended before any code was written. Everything below is against the
amended card: three element draws, on two kinds of surface.

### `render`

- `voe_render_frame_draw_elements` takes `first` and `count`. A range past what
  was submitted returns false and names the numbers, written as a subtraction so
  the comparison cannot wrap. A count of nought records nothing and is not a
  refusal.
- `voe_render_frame_elements_submitted` is the count a caller reads either side
  of its own submissions. Named so it cannot be read as
  `voe_render_frame_draw_count`; its header says why the two are never the same
  number.
- **The range reaches the shader as `firstInstance`, and that needed a shader
  change nobody had predicted.** Slang gives `SV_InstanceID` HLSL's meaning — the
  instance's number *within the draw*, counting from nought whatever
  `firstInstance` was — so the first version drew the first `count` records of
  the buffer for every range but the first. It looked perfect with one surface in
  the frame. `shaders/elements.slang` now reads `SV_StartInstanceLocation`
  alongside it and adds the two. `two_ranges_two_matrices_two_draws` is written
  to catch exactly that: the same two rectangles in both ranges, only the matrix
  moved.
- `voe_render_element_surface_matrix(size)` is new and is the piece `render`
  owns: element millimetres, y down from the top-left, onto the surface's own
  plane in metres, y up, **centred on the surface's origin**. Centred rather than
  cornered because the card says the size is what the millimetre-to-metre step
  needs, and a step that only scales would not need it.
- **`voe_render_element_transform` is now a composition of it**, as the card
  recommended: an orthographic step over the surface's own metres, times the
  surface matrix. Both scales in that step are positive. **There is exactly one
  negation on the element path and it is `m.m[1][1]` in
  `voe_render_element_surface_matrix`** — greppable, and checked.
- `voe_render_element_surface_size(target, pixels_per_millimetre)` divides both
  axes by the one number.

### `3d`

- `voe_3d_panel`: `first`, `count`, `size` (millimetres), `layer`. No matrix —
  the `voe_scene_transform` row under the same entity is where placement lives.
  The header's first paragraph is *"THE RANGE IS ONE FRAME'S AND IS WRONG THE
  MOMENT THAT FRAME HAS ENDED."*
- `voe_3d_panel_set_range` is the only write after creation; size and layer
  travel through it untouched, the same shape `voe_3d_mesh_set_geometry` has.
- **The draw system walks two tables.** Meshes first — nothing hangs on that
  order and the header says so — then panels, every one of which is held back.
  `struct deferred` became a tagged union: a `bool panel` and two arms, a mesh's
  geometry and record or a panel's composed matrix and range. `hold` now takes
  the world matrix separately, because the two arms keep their matrices in
  different places. `draw_group` issues whichever arm it finds, from the one
  sorted list. `voe_3d_depth_sort` was not touched.
- **A stale range is skipped**: `range_is_this_frame_s` compares against
  `voe_render_frame_elements_submitted`. **What it does not catch**: a stale range
  that happens to fall inside what some other surface submitted this frame draws
  somebody else's rectangles. No frame stamp was added — it is a field, a write
  and a comparison every frame for a case nothing has hit, and the source says so
  where the next reader will look.
- Groups are sized `meshes + panels`. `struct group` gained a `capacity` and
  `hold` asserts against it — see *the analyser* below.
- **A world that is drawn must now register the panel table even with no panels
  in it.** `3d/tests/import.c` grew that line and `draw_system.h` states the rule
  beside the camera-and-light one.

### `dev`

Three element surfaces and 3 draw commands, exactly as the amended card's table
says:

| Surface | How it reaches the frame | Content |
|---|---|---|
| exhibit | entity + panel, `WORLD` | the 40 rectangles and 40 letters, unchanged |
| badge | entity + panel, `OVERLAY` | 5 rectangles, violet, deliberately asymmetric |
| `src/surface.c` | direct `voe_render_frame_draw_elements` | 9: a plate, a square and six ticks |

- `src/elements.c` no longer draws. `voe_dev_elements_transform` is gone.
- `VOE_DEV_UI_SCALE` lives in `src/surface.h`, default `1.0f`, and a person
  changes it by editing that line. `surface.c` computes
  `target.height / VOE_DEV_SURFACE_HIGH * VOE_DEV_UI_SCALE` and hands it to
  `voe_render_element_surface_size`.
- **The header sentence about what a millimetre is**, in `src/surface.h`: *"On a
  panel, an authored millimetre is a real millimetre: 240 mm is 0.24 m and a
  ruler would agree. On this surface it is a PROPORTION OF THE SURFACE'S AUTHORED
  HEIGHT — nothing physical, and nothing to do with any display."*
- The stretching paragraph in `src/elements.h` is **deleted**, not rewritten, and
  the "fewer millimetres of width / cut off" sentences are in `src/surface.h`.
- The exhibit stands at (0, 1.2, −2.2) with a transform scale of 14, so 240 mm ×
  135 mm is 3.36 m × 1.89 m in the world; the badge is at the turning cube with a
  scale of 16. Those scales are the entity's transform doing what a transform
  does — the authored millimetres were not touched.

### What was checked, and on what

`cmake -P check.cmake` exits zero on **Linux** (Fedora 44, clang 22.1.8, CMake
4.3.0, Ninja 1.13.2, slangc 2026.13.1, NVIDIA RTX 4070 Laptop, Vulkan 1.4.341) —
all steps, 38 tests, analyser clean. **Windows was not checked and has no machine
here**; nothing in this card is platform-specific — no `#ifdef` was added and no
platform surface was touched — so what the other machine turns up becomes a new
card.

The analyser was not clean at first and the warning was real: with the groups
sized `count + panel_count`, it could no longer prove that a group a mesh is held
in has any room. `struct group` now carries its `capacity` and `hold` asserts
`deferred != NULL && count < capacity`. That is the card's *"keep its arithmetic
honest"* made checkable rather than argued.

Looked at, not asserted:

- The exhibit standing in the world with the figure and a sprite in front of it,
  the violet badge over everything, and the plate and six ticks in the corner.
- The same scene in a 430 × 540 window: **the ticks are the same size and the
  same spacing and the last two have fallen off the right edge**; the white square
  is still square; the world panel and the badge are unchanged, because a resize
  changes only the camera's aspect. That is ADR-0104's claim and it is the picture
  the amendment asked for.

### Draw counts

**30 before, 32 after** — 29 meshes throughout; one element draw became three.
The "before" was measured by building `HEAD` in a scratch worktree and running
it, not inferred.

### What was deliberately not built

No widgets. No `ui` edge — there is no `ui` anywhere in this diff, and `3d`'s
`DEPENDS` line is unchanged, as are `render`'s and `dev`'s. No picking. No frame
stamp on the panel component. The element record is untouched — still eighty
bytes, still the same fields, and `tests/elements.c` still asserts it. The mesh
path, the four groups, the depth clear and the order they are issued in are
unchanged.

### One Verify bullet could not be performed as written

*"a scale of nought is the caller's bug and is refused rather than producing an
infinity"* — the two halves of that sentence ask for different things. Rule 13
says a caller's bug is an assert, `voe_render_element_transform` already asserts
on the same shape of mistake, and an assert cannot be asserted on from a test
that has to keep running. So it is `VOE_BASE_ASSERT(pixels_per_millimetre >
0.0f, ...)`, negatives included, and the test covers the other three items in
that bullet. Flagging it rather than quietly picking one reading.
