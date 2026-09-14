# 0086. Render layers are the developer's to create and order — direction, not a mechanism

- **Status:** Accepted
- **Date:** 2026-09-07
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Amended:** 2026-09-07, same session — the concern about occlusion was withdrawn by the principal and the withdrawal is recorded in place of it. The direction is unchanged.
- **Amends:** ADR-0074 (the layer *count*, not its projection)
- **Closes:** D-123

## Context

The principal stated a destination, unprompted and ahead of the board:

> I think it would be nice to have render layers. Where dev can add and remove
> render layers. So if dev put a gui or 3d in another render layer it is
> guaranteed to be rendered on top of other layers (or below depending on order).

**This contradicts a sentence already written in the engine**, and that is the
reason this ADR exists rather than a register row.
`3d/include/3d/mesh_component.h` says:

> The two layers a drawable can be in. **There are two and a third is a decision
> rather than a parameter** — a layer costs a depth clear and an ordering rule
> that has to be written down, and neither of those is something to grow by
> counting upwards.

That sentence was right about the *cost* and is now wrong about the *destination*.
Left alone it would mislead whoever writes the GUI cards, which is the next
planning job.

Constraints already fixed, and most of the design dies inside them:

- **ADR-0074 — the overlay is still in 3D and still in metres**, seen through the
  same camera. Not orthographic, not screen space. That shape was recommended by
  the tech lead and rejected by the principal, on the ground that screen space is
  the one form a headset could never present and would have become the easy
  default long before anyone noticed. **Nothing here reopens that**, and layers
  must not become the back door to it.
- **ADR-0049 — everything is in 3D space.**
- **The mechanism already exists and is one call.**
  `voe_render_frame_clear_depth` clears depth where the frame stands without
  touching colour, inside the frame's single rendering block — there is no second
  pass to open or tear down. A layer boundary *is* that call. Card 024 built it,
  and its cost is one full-target depth clear per boundary.
- **ADR-0061 / ADR-0081 — within a layer, opaque first in table order, then
  blended sorted furthest away first.** Layers run *across* those two passes
  rather than inside them, so N layers is 2N groups and N−1 clears.
- **ADR-0034 / ADR-0055 — implement on demand; nothing is built that nothing
  measures.**
- **ADR-0083 — a module produces things to draw; a switch changes how drawing
  happens**, and the sharper test: *would a developer who wants this have to
  modify the engine? Then it is a switch.*

## The principal's own rule argues for this, which is worth saying plainly

Under ADR-0083's test, a fixed layer count is a **fork waiting to happen**: a
developer who needs a third layer has to modify the engine, and the switch list is
the anti-fork list. Making the count a parameter is not scope creep on the core —
it is precisely the move that keeps someone from forking us. The instinct and the
rule agree, and the earlier two-layer sentence is the one out of step.

## Direction

**Layers are the developer's to create, order and remove.** The fixed enum of two
is the wrong long-term shape and stops being the destination as of this ADR.

What this pins, as destination:

1. **A layer is created, not enumerated.** A program says how many it wants and in
   what order; `WORLD` and `OVERLAY` become the two a program gets by default, not
   a closed set.
2. **A layer boundary is a depth clear and nothing else.** That is the whole
   mechanism, it already exists, and this ADR does not invent a second one. Order
   is the order the layers are drawn in.
3. **Every layer is still 3D, still in metres, still the same camera.** No layer
   gets its own projection, no layer is screen space, and a layer is not a place
   to smuggle in the orthographic overlay ADR-0074 refused. **A layer changes
   *when* something is drawn. It has never changed *how*, and it does not start
   now** — the same sentence that already governs the two.
4. **Inside a layer, nothing changes.** Opaque then blended, sorted furthest away
   first, lighting from the material. The rules that hold across two layers hold
   across ten.
5. **Nothing is built until a card needs it.** This is direction, in ADR-0073 and
   ADR-0077's sense. No API, no card, no count.

## The concern I raised, and its withdrawal — amended same session, 2026-09-07

**Recorded rather than deleted, because it is the kind of concern that gets raised
again by whoever reads this next.**

As first written, this ADR argued that a layer guaranteed on top cannot be
occluded, that a panel bolted to a wall would therefore draw through it, and that
this is what breaks presence in a headset — with a guard attached reading *layers
order, they do not excuse placement.*

**The principal withdrew it in one sentence and he is right:**

> Well, the layers are only render layers. Made for overriding draw order. So
> anything wrong in layers would be wrong without them as well.

That is correct, and here is why it is not a close call. **The capability already
exists.** `VOE_3D_LAYER_OVERLAY` is already drawn after the world's depth is
cleared and already cannot be occluded by anything in the world — a developer who
wants to put a wall-mounted panel on top can do it today, with two layers and no
change to anything. Going from two to N does not introduce a failure mode; it
makes more instances of an existing one addressable. **The concern was aimed at
the overlay concept, not at the count** — and that concept is ADR-0074's, taken
deliberately and not reopened here.

**And the guard was already written, in the right place.** Card 023 states that
both placements are first-class, that a panel on a wall and a panel always in
front are equally ordinary, and that *"a widget system that only really works in
one of the two placements has broken the decision without anybody editing it"* —
which it names as the constraint that card most has to respect. Restating it here
as a consequence of layer *count* attached an existing rule to the wrong decision
and would have left two copies of it drifting apart.

**What survives:** the cost. Each boundary is a full-target depth clear. A handful
is nothing; at a high resolution, dozens is real bandwidth spent on ordering. The
count is the developer's and so is the cost, but the engine should be able to say
what it costs rather than leaving them to find out. That is D-126.

**Nothing else survives, and the direction is unchanged.**

## Blast radius

**Cheap today, because nothing is built.** This is a destination and an amendment
to a comment, taken before the cards that would have been written against the old
sentence.

What it makes expensive later: a public layer type that programs put in their
components. Once drawables name a developer-created layer, changing what a layer
*is* touches every program. That is the decision this ADR deliberately does not
take — the mechanism stays open and only the destination is fixed.

Reversibility: **cheap now, load-bearing once a card builds it.**

## Consequences

- **The GUI cards can be cut against this** rather than against a two-layer enum
  that was about to be wrong.
- **`3d/include/3d/mesh_component.h`'s comment is now stale in one clause** and
  must be corrected by the first card that touches it. Its cost reasoning stays,
  its *"a third is a decision rather than a parameter"* does not. Engine code is
  not edited from this root, so this is a card's job, not this ADR's.
- **ADR-0074 is amended in its count and untouched in its substance.** The overlay
  is still perspective, still metres, still the same camera. That was the load
  bearing half and it is not reopened.
- **A cost nobody pays today becomes payable on request**, which is the shape
  ADR-0066 asks for: nothing turns a cost on by default and a developer opts in.
- **The consequence I do not like** — as first written, this said that handing out
  an ordering override before anyone has experience of it is a risk. **Struck the
  same session**: the override is not handed out here, it already exists as the
  overlay, and this ADR only makes the count a parameter. See the amended section
  above. What is left in its place is smaller and real: **the engine will know less
  about what a layer means** once programs create their own, where today it can
  reason about "the overlay" as a named thing. That bears on D-127, which asks
  whether the offscreen panel is a layer or something else.

## Rejected options and why

**Keeping two layers and adding a third when asked.** This is what the current
comment proposes, and it is the fork-shaped answer ADR-0083 argues against: every
program that needs a fourth waits for us or edits us.

**Layers with their own projection or in screen space.** Refused by name, again.
ADR-0074 settled it, the tech lead proposed it once and was overruled, and a
per-layer projection is the same proposal wearing a parameter.

**Deciding the mechanism now** — integers versus created objects, what removal
does to the drawables inside a layer, whether two layers can share a depth clear.
All real, none decidable without the GUI cards that do not exist. See below.

## Questions this opens

- **D-124 — how a layer is named and ordered**: a sparse integer a drawable
  carries, or a created object with an explicit position. Trigger: the first card
  that needs a third layer.
- **D-125 — what happens to drawables in a layer that is removed.** Trigger: the
  same card.
- **D-126 — whether two adjacent layers may share one depth clear**, which is
  grouping rather than layering and is how the cost is kept down at a high layer
  count. Trigger: the first program with enough layers for the clears to show in
  the frame time.
- **D-127 — whether the offscreen panel is a layer or a different thing.** Card
  023 builds a panel as its own render target; that is composition, not ordering,
  and the two will be confused if nobody writes down which is which. Trigger: the
  first GUI card.
