# 0099. The GUI surface is one space, and it runs Y down from the top-left

- **Status:** Accepted
- **Date:** 2026-09-09
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Closes:** D-169

## Context

**Two cards describe the same space and they disagree about which way Y points.**

`voe_render_element`, which landed with card 030 and is tested and in the tree,
says:

> **Y RUNS DOWN.** The origin is the surface's top-left corner and y increases
> towards the bottom, which is what every interface in the world means by a
> coordinate.

Card 033, being implemented now, says the opposite for the folder that will build
those records:

> **Millimetres, two dimensions, X right, Y up, origin at the panel's
> bottom-left corner** … *A column lays its children from the top down — the
> first child called has the greatest Y.*

Both sentences are about *the panel's own two-dimensional space in millimetres*.
They cannot both stand, and the seam between them is card 034, where layout's
rectangles become element records.

**How it happened, because it was not carelessness.** ADR-0033's deciding factor
is quoted in its own decision: *"`text`, `sprite` and `ui` are on the roadmap and
are Y-up by nature, and an engine in which 'up' means Y in four folders and Z in
five is a tax paid forever."* Card 033 honoured that sentence. Card 030 was
written later, by which time there was an actual interface space to describe, and
its coder argued the other way — in the record's header, at length, and correctly
for what it was describing.

Constraints already fixed:

- **ADR-0033 point 6** — *exactly one Y flip exists in the engine, and it lives in
  the viewport*, and point 7 — *the rule is the invariant, not the sign*. Flipping
  twice is named there as the classic form of this bug, *invisible until it isn't*.
- **ADR-0089** — the GUI is authored in millimetres, physically.
- **ADR-0093** — `ui` receives the pointer as **a two-dimensional point in
  millimetres in the panel's own space**, produced by whoever owns the camera. So
  the space is crossed in *both* directions: rectangles go out, a pointer comes in.
- **`voe_render_element`'s space is shipped**, and `voe_render_element_transform`
  is where its sign lives. `render/tests/elements.c` pins it with a deliberately
  asymmetric arrangement, because *a symmetrical one would hide an upside-down
  picture entirely*.
- **`text`'s glyph box is in ems with +y up** (`struct glyph`), and that is a fact
  about a font file rather than a choice — it is converted once wherever glyphs
  become elements, whichever way the surface points.

## Options considered

### Option A — `ui`'s space *is* the element record's space: Y down, top-left origin

`ui` lays out in the same space it emits into. Emission is a copy; clip rectangles
are copies; the pointer arrives in the space layout already works in. A column's
first child has the *smallest* Y, which is also the order it was called in.

One conversion remains and it is unavoidable in every option: `text`'s y-up em box,
turned round inside the one function that places glyphs.

### Option B — `ui` keeps Y up, bottom-left, and converts at the boundary

Card 033 as written. ADR-0033's sentence about `ui` is honoured literally. The
conversion `y' = height − y − h` is applied at emission to every rectangle and
every clip rectangle, and applied the other way to the incoming pointer point.

### Option C — turn the element record round instead, so the whole engine is Y up

`render`'s element space becomes bottom-left, Y up, and
`voe_render_element_transform` loses its negation.

## Decision

**Option A, taken by the principal on 2026-09-09 the day it was put to him, with
card 033 in flight. The deciding factor is the number of places the conversion has to
be remembered.** Under A there is one, in the glyph emitter, where a font's own
convention is turned round and where it would exist anyway. Under B there are
three — rectangles, clip rectangles, and the pointer coming back the other way —
and they are the *worst* kind of three: arithmetic that must be applied to some
values and not others, in a folder whose output nobody can see until two cards
later.

The second factor: **the engine already answered this question in code.** The
element record's space is shipped, tested and argued in its own header. A GUI
folder that authors in a different space than it emits into is not honouring a
convention, it is holding two.

**ADR-0033 is not weakened by this and its point 6 is the reason for it.** That ADR
fixes the axes of *the world* — a panel's own placement is still Y-up in metres,
from the `voe_scene_transform` row under the same entity id as its panel row. What A settles is the
*parameter space of a flat surface*, which the world's handedness never described,
and it settles it so that the reconciliation between an interface's coordinates and
Vulkan's clip space stays in the one function that already owns it. The tax
ADR-0033's deciding factor warned about is *two meanings of up in one engine*; A
gives the GUI one meaning of down, in one place, and B is what would spread it.

## Blast radius

**Cheap today, and it stops being cheap almost immediately.** `ui` holds exactly
one card's worth of code, being written now, and `ui/include/ui/layout.h` does not
yet exist. The change is a paragraph of the header, the direction a column
accumulates, and one test's expectation.

After card 034 emits, 035 clips and 036 themes, the same change is every rectangle
in the folder plus every test that asserts one — and after a program outside the
engine has laid out a panel, it is a breaking change to somebody else's code.

Reversibility: **cheap this week, load-bearing by 035.**

## Consequences

- **Card 033 was amended in flight**, the same day, and the amendment is dated at
  the top of the card so the coder holding it can see what changed and why. That is
  the one thing the card rules otherwise avoid; it is cheaper than the alternative
  exactly once, and the reason it was worth it is that the header being written that
  afternoon is the one that would have carried the wrong sentence for a year.
- **Emission on card 034 is a copy**, and the hit test on the same card compares the
  pointer against rectangles in the space it arrived in.
- **A column's first child has the smallest Y**, which reads the same way the calls
  do. The hazard card 033 lists as *"Y. Top-down flow in a Y-up space"* disappears
  rather than being tested for.
- **The consequence I do not like:** ADR-0033's deciding factor names `ui` as Y-up
  and a reader will find that sentence before this one. So this ADR is named in the
  register row and `ui`'s own header must say, in one line, that a surface's
  parameter space is not the world's and why.
- **`text` is untouched.** Its box stays in ems with +y up, because that is what a
  font says.

## Rejected options and why

**Option B — keep Y up and convert at the boundary.** Three conversion sites, one
of which runs the other way, and none of them visible in a test until 034. It also
leaves the folder's internal space different from the only space its output is ever
seen in, which is a thing every future card has to hold in its head.

**Option C — turn the element record round.** It is shipped, tested, and its header
argues the case well; and it would put the GUI's coordinates at odds with every
interface toolkit a person has used. Changing working, reasoned code to satisfy a
sentence in a planning document is the wrong direction of fit.

## Questions this opens

- **D-170 — whether `sprite`'s and any future flat surface's authoring space
  follows this or `text`'s.** Nothing asks yet: `sprite` is world-space quads in metres and
  has no parameter space of its own. Trigger: the second flat surface with its own
  coordinates.
