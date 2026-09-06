# 024 — the overlay layer

status: review
claimed-by: claude-opus-5 (kanban-coder)
blocked-by: 021b

Written by the tech lead at the principal's direct request (2026-09-06), which is
an exception to *the human writes the card* given for this decision and not
standing.

**This card exists because a decision had no home.** ADR-0074 answers the question
card 021b escalated — the heads-up line disappearing inside geometry — and card
023 is the card that question was filed against. 023 is blocked on 022 and on
text that can change, both far off, and the defect is visible now. So the work
gets a card of its own rather than waiting, and rather than growing 023.

## The scope this card was missing

**Amended 2026-09-06, mid-card, after the coder stopped and asked.** The card
named the trap — *a capability `render` does not have gets a card in `render`* —
and then did not say which side of it the depth clear fell on, or put `render` in
scope. It listed `3d` and `dev` and nothing else. **That is the card's fault, not
the coder's**, and stopping to ask was the right move rather than a delay; a
guess here would have been `3d` recording a Vulkan command, which is the one
thing the bullet existed to prevent.

**It does not become two cards.** The rule about a gap in `render` is about where
code lives, and the call lands in `render` either way. Splitting would buy a
second claim and a second review for one function that carries no decision of its
own, has no consumer but this card, and cannot be verified except by the overlay
that uses it. **No new ADR either** — `render`'s public surface is by id and
**grown on demand** by an existing decision, and `3d` already depends on
`render`, so no module edge is added and nothing here is a decision.

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
- **`render` grows one call: a depth clear inside the open frame.** *Added
  2026-09-06, after the card was claimed — see "The scope this card was missing"
  below.* The overlay needs depth cleared **mid-frame**, and `render` is the only
  folder that may record a command. Shape:

  ```c
  void voe_render_frame_clear_depth(voe_render_device *device);
  ```

  - **`vkCmdClearAttachments` inside the rendering block that is already open.**
    Not a second `cmd_begin_rendering`, not a second target, not a barrier. The
    frame opens exactly one rendering block and this records into it, so ADR-0051's
    shape is untouched and there is nothing to tear down or resume.
  - **Depth aspect only, colour untouched**, over the whole render area — the same
    rect the scissor already uses.
  - **The clear value never leaves `render`.** It is the same
    `VOE_RENDER_DEPTH_CLEAR` the frame's load op already uses, read from the same
    constant. `3d` does not learn the number and must not be given a parameter for
    it; depth runs backwards here and a caller-supplied value is a way to get it
    wrong that this signature simply does not offer.
  - **It asserts if no frame is open**, in the voice `voe_render_frame_draw`
    already uses for the same mistake.
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
- **Reaching around `3d` into `render`.** `3d` records no Vulkan and learns no
  Vulkan. The depth clear is a `render` call because recording commands is
  `render`'s alone — that is the rule, and it is satisfied by *where the code
  lives*, not by which card it was written on. If anything else in this card
  turns out to need a capability `render` lacks, **stop and report it** rather
  than reaching through.

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
- **The depth clear is one call into the open rendering block**, and the frame
  still opens exactly one. If the implementation ended up closing and reopening
  rendering, say so and why — that is a different shape and it needs saying.
- **Nothing outside `render` names the depth clear value.**
- `check.cmake` at zero, and say which platform it was verified on.

## Report, do not decide

- **Whether an explicit order number is wanted inside the layer.** Distance
  ordering works because overlay elements have real positions, but a UI author
  usually wants to say *this is in front of that* outright. If this card makes
  that feel missing, that is a finding and it is already an open question.
- **Where the overlay pass should sit once post-processing exists** — before or
  after. It has no answer today because there is no post-processing, and the
  question is real in both directions.

## Notes — what was built and what was verified

**Verified on Linux (Fedora, KWin/Wayland, NVIDIA RTX 4070 Laptop, Vulkan 1.4.341).**
Windows unchecked; nothing here is platform-specific — the one new Vulkan command
is core 1.0 and no backend file was touched.

`cmake -P check.cmake` exits zero: 31 tests, 86 files analysed, every folder
still configures standalone.

### What the three scope bullets became

- **The layer is a field on the drawable.** `voe_3d_layer` and a `layer` field on
  `voe_3d_mesh` — the mesh component is the drawable, and it is neither the
  material nor the transform. `VOE_3D_LAYER_WORLD` is nought because a C struct
  has to have some value when nobody wrote one; the header says in as many words
  that this is not a claim that the world is the normal case, and every call site
  in `dev` and in `import.c` names its layer outright rather than leaning on the
  zero.
- **`render` grew exactly the call the card specified.** `void
  voe_render_frame_clear_depth(voe_render_device *)` — one
  `vkCmdClearAttachments`, depth aspect only, whole render area, the same
  `VOE_RENDER_DEPTH_CLEAR` the load op reads, asserting when no frame is open. It
  takes no clear value and `3d` is never told the number.
- **The draw walks four groups.** world-solid (issued during the walk, table
  order) → world-blended (sorted) → depth clear → overlay-solid → overlay-blended
  (sorted). Both sorts are the existing `voe_3d_depth_sort`, unchanged.

### The two calls the card left to this card

- **The overlay has a solid group from the start.** The card asked which and why.
  It is there because the layer and the alpha mode are separate axes: without it
  an opaque drawable marked overlay would silently draw in the world, which looks
  like nothing is wrong rather than like a missing feature. It costs one branch in
  a walk that already happens, and `dev` now has an opaque overlay quad, so the
  group is exercised rather than merely present.
- **An empty overlay clears nothing.** The clear is skipped when both overlay
  groups are empty. A full-screen depth clear is real work and a program with
  nothing above the world should not pay for it every frame; the depth image is
  thrown away at the end of the frame either way, so nothing observable changes.
  Flagging it because it is a decision the card did not make.

### What `dev` shows now

The heads-up line moved to the overlay. The sign stayed in the world, on both
faces, so the demo still shows something that *is* occluded. Three new quads
stand inside the turning cube in the overlay, sharing the existing quad geometry:
an opaque lit one in the middle, a blended lit one at +Z and a blended unlit one
at −Z. Between them they carry every remaining Verify item — the solid group, the
sort inside the layer, and the lighting axis — and being inside a solid cube is
what makes "nothing in the world covers it" continuously watchable instead of
something you have to fly into a wall to see.

### Verified, and how

- **Fly into a cube; the line stays on top.** Reproduced without input by
  temporarily driving the orbit through the geometry (`ORBIT_RADIUS` 0.6,
  `ORBIT_HEIGHT` 0.0), screenshotting, and reverting. With the camera inside the
  cube and its texture filling half the frame at point-blank range, the line is
  drawn whole and readable over it.
- **The layer is what does it, proved by removing only the clear.** With
  `voe_render_frame_clear_depth` commented out and nothing else changed, the same
  camera position loses all three overlay quads behind the cube; putting it back
  brings them back. That A/B is the strongest single piece of evidence here.
- **The world object still gets occluded.** The sign disappears behind the cube at
  close range in the same runs in which the line does not, so the layer is a
  property of a drawable and not a switch the frame is in.
- **Two overlapping blended things in the overlay order correctly, and the order
  swaps.** The two blended overlay quads read orange–purple–green from one side of
  the orbit and green–purple–orange from the other, and the opaque one between
  them hides whichever blended one is behind it.
- **The layer did not pick up a lighting meaning.** Measured rather than eyeballed
  across nine frames of a lap: the lit overlay quads track the sun (peak colour
  moving between (170,109,195) and (112,76,127), and vanishing entirely when the
  sun is on the far side), while the unlit heads-up line holds (196,218,227) in
  every frame it is over the clear colour. Sunlight still does not touch text and
  the text still arrives at the tint it asks for — no second multiply.
- **Nothing in the world changed.** The pre-card build and this one were run to the
  same elapsed time and diffed: 1.5% of pixels differ, and the mask shows that
  difference is the three new overlay quads plus one-pixel antialiasing jitter on
  every edge, which is two independently-timed runs and not a change in the
  picture. Cube interiors, the models, the world's blended quads and the
  background are identical.
- **The depth clear is one call into the open rendering block.** The frame still
  opens exactly one: `cmd_begin_rendering` and `cmd_end_rendering` appear once
  each in `frame.c` and nowhere else in that path. Rendering is not closed and
  reopened.
- **Nothing outside `render` names the clear value.** `VOE_RENDER_DEPTH_CLEAR`
  appears only in `render/src/device_internal.h` and `render/src/frame.c`.
- **The Vulkan usage is clean.** The debug build runs with the validation layer,
  and with synchronization validation and best-practices additionally enabled
  there are zero `SYNC-` findings and no message mentioning `ClearAttachments`.

### Markers left

None. No `DEVIATION:` and no `BLOCKED:` — the amended card put `render` in scope,
which was the one thing that would have produced either.

## Report, as the card asked

- **An explicit order number inside the layer is not missed yet, and would be the
  moment a panel is built.** Distance ordering worked here with nothing added,
  because the three overlay quads have real positions and stand at different
  depths. But placing them meant choosing `OVERLAY_QUAD_Z` — a number whose only
  job is to decide what is in front of what — and that is the shape of the
  problem: a UI author who wants a panel over a panel has to express it as
  millimetres of separation and then keep every element's depth consistent by
  hand. It did not block anything on this card. It is already an open question and
  this is a data point for it, not a new finding.
- **Where the overlay pass goes once post-processing exists — still no answer, and
  the question got slightly sharper.** The overlay is now the last thing written
  into the colour target before it is blitted out. Anything inserted between the
  drawing and the blit therefore processes the overlay along with the world by
  default, which is the wrong default for a crosshair and the right one for a
  scene. Both directions are real and nothing here decides it.
- **Two layers was enough and nothing wanted a third.** No point in this card
  wanted one.

### One thing found on the way past, not fixed

Running with best-practices and synchronization validation turned on surfaces
pre-existing `vkCmdPipelineBarrier2` warnings — image barriers using
`VK_ACCESS_2_MEMORY_READ_BIT|VK_ACCESS_2_MEMORY_WRITE_BIT` where the layout wants
a narrower mask (`SHADER_READ_ONLY_OPTIMAL`, `TRANSFER_DST_OPTIMAL`,
`TRANSFER_SRC_OPTIMAL`), plus "fully consumed by the buffer" sub-allocation
advice. None of them is from this card — no barrier was added or changed — and
neither validation mode is on by default. Reported rather than repaired, and it
is a `render` card if it is worth one.
