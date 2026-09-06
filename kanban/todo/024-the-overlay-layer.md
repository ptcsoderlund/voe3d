# 024 — the overlay layer

status: todo
claimed-by: -
blocked-by: 021b

Written by the tech lead at the principal's direct request (2026-09-06), which is
an exception to *the human writes the card* given for this decision and not
standing.

**This card exists because a decision had no home.** ADR-0074 answers the question
card 021b escalated — the heads-up line disappearing inside geometry — and card
023 is the card that question was filed against. 023 is blocked on 022 and on
text that can change, both far off, and the defect is visible now. So the work
gets a card of its own rather than waiting, and rather than growing 023.

## Goal

An object can say it belongs **above the world**, and then nothing in the world
covers it. Fly into a cube; the writing stays on top.

## What is decided, so none of it is a choice

All of this is ADR-0074 and is summarised here so this card can be implemented
without reading it.

- **The overlay is not screen space and not orthographic.** Its objects keep real
  positions in metres and are seen through the **same camera** as everything
  else. No orthographic projection is added anywhere in the engine by this card.
  *"Always on top" and "screen space" are two different things and only one of
  them is refused* — those are card 021b's words and they are the whole of it.
- **Ordering is a depth clear between the world and the overlay**, not a depth
  range split and **not the depth test turned off**. Both alternatives were
  considered and rejected: the split spends the depth precision the reversed-depth
  convention exists to buy, and turning the test off stops the layer's own objects
  occluding *each other*, which one line of writing does not notice and a GUI of
  overlapping panels does. Card 021b found that second argument; it is why this
  shape and not the cheaper one.
- **Inside the layer, nothing is different.** Same shader, same material records,
  the same three alpha modes, and the blended pass's existing rules — depth test
  on, depth write off, sorted back-to-front per object — apply within the overlay
  exactly as they do within the world. Overlay elements have real positions, so
  the existing sort orders them correctly with no new key.
- **Both placements are first-class and neither is the default.** An object in the
  world that gets walked behind, and an object above the world, are equally
  ordinary. Do not implement this as a flag that makes overlay the easy path.
- **The layer decides order and nothing else.** It does **not** decide lighting.
  Unlit is already a material property and text already sets it; an overlay
  object that wants the sun on it is simply a material that is not unlit. Adding
  any shading meaning to the layer is out of scope and would be a finding to
  report, not a convenience to take.
- **Two layers is the cut**: world and overlay. A third is a decision, not a
  parameter — if this card finds itself wanting one, report it.

## Scope

- **A way for an entity to say which layer it is in.** Where this lives is
  `3d`'s to decide — `3d` owns the drawing components — but it is a property of
  the drawable, not of the material and not of the transform.
- **The draw walks the world, clears depth, then walks the overlay.** The world's
  opaque and blended groups as today; a depth clear; then the overlay's own
  opaque and blended groups. Text is blended, so the overlay's blended group is
  not optional. Whether the overlay's opaque group is worth having from the start
  is this card's call — say which you did and why.
- **The clear is depth only.** Colour is loaded, not cleared; the world's picture
  has to survive.
- **`dev` shows both.** The heads-up line goes in the overlay layer and stops
  disappearing. Something else — a quad, a cube, anything — stays in the world
  and still gets occluded, so the demo proves both placements rather than just the
  new one. `dev/src/main.c`'s header says what to look for and what it means when
  it is wrong, in the voice the rest of that file already uses.

## Not in scope, and each has a reason

- **Head-locked placement.** `facing_the_camera` in `dev/src/main.c` stays as it
  is on this card, thirty lines and all. Making *"I sit this far in front of the
  camera"* the engine's job rather than the program's is decided in principle and
  its shape is still open — a component resolved by a system between the camera
  and the transform drain, or a flag read at draw time. It is a card of its own
  once that is settled. **Do not fold it in.**
- **Any orthographic projection.** Named because it is the obvious thing to reach
  for and it is refused: it is the one form a headset cannot present, and that is
  why this decision came out the way it did.
- **A third layer, layer masks, a second camera, a second target.** The engine
  asserts on more than one camera and that stays true after this card.
- **The offscreen panel.** Still card 023's to build.
- **Text that changes.** Untouched by this; getting in front of the world was
  never why the frame-rate readout cannot be drawn.

## Where this is likely to go wrong

- **Clearing depth in the wrong place**, so the world's picture goes with it. The
  colour attachment loads and only depth clears.
- **The depth clear value.** Depth runs backwards here — cleared to 0, compared
  `GREATER`, near at 1.0. The overlay's clear is the same clear as the frame's,
  not a different constant, and getting it wrong gives a layer that draws nothing
  or one that never occludes itself.
- **Assuming the overlay does not need a sort.** It does, the moment there are two
  blended things in it, and it is the sort that already exists.
- **Letting the layer imply unlit.** It is a separate axis and the engine already
  has two places it could drift between.
- **Reaching around `3d` into `render`.** A capability `render` does not have gets
  a card in `render`, never a reach-around.

## Verify

- Fly the demo into a cube. **The heads-up line stays readable and on top.** Nine
  metres out or one metre out, it does not matter — that was the diagnostic card
  021b used and it should now be uninteresting.
- **The world object still gets occluded.** If everything is on top, the layer is
  a global switch rather than a property, which is the failure this card is shaped
  to avoid.
- **Two overlapping blended things in the overlay order correctly against each
  other**, and swapping which is nearer swaps which wins.
- Sunlight still does not touch text, and a non-unlit material in the overlay
  still *is* lit — the layer changed order and nothing else.
- `check.cmake` at zero, and say which platform it was verified on.

## Report, do not decide

- **Whether an explicit order number is wanted inside the layer.** Distance
  ordering works because overlay elements have real positions, but a UI author
  usually wants to say *this is in front of that* outright. If this card makes
  that feel missing, that is a finding and it is already an open question.
- **Where the overlay pass should sit once post-processing exists** — before or
  after. It has no answer today because there is no post-processing, and the
  question is real in both directions.
