# 0074. Overlay content is a layer above the world, still in perspective and still in metres

- **Status:** Accepted
- **Date:** 2026-09-06
- **Deciders:** Human, Tech Lead
- **Confirms:** ADR-0049, which survives this challenge unamended
- **Supersedes:** —
- **Superseded by:** —
- **Amended 2026-09-06, same day:** Option E, the credit in *Context*, and the corresponding rejected entry were added after reading card 021b's report, which the tech lead had not seen when this was first written. **No part of the decision changed** — the report's argument supports the option already chosen.

## Context

D-056, arriving ahead of the card it was filed against. It was written down as
card 023's blocker — *how does a UI quad sit above the world without a separate
rendering path* — and card 021b brought it forward by putting the first
camera-locked text on screen, where the principal met it as a defect rather than
as a question:

> *"The gui or hud layer is in 3d space, which makes them disappear in 3d models
> near the camera. We need a layering system where hud and gui is on an
> orthographic layer which does not take directional lighting and 3d effects
> unless specifically told to do so."*

**Half of that ask already existed and the tech lead said so first.** Unlit is a
material property (ADR-0071) and `dev/src/main.c` already sets it on the text
material; a glyph in this engine is not lit by the sun today. The directional
light was never the defect, and keeping the decision on the one real question is
what made the rest of the session cheap.

**The real defect is occlusion, and it has a second half nobody had named.**
`facing_the_camera` in `dev/src/main.c` places the heads-up line at
`HUD_DISTANCE` in front of the eye, in world space, so anything nearer covers it.
It also costs thirty lines that re-derive that position every frame, wedged
between the camera system and the transform system, carrying a comment about a
whole mouse movement of jitter if it is placed anywhere else in the loop. Both
symptoms come from the HUD being a world object that has to be chased.

**The tech lead's first recommendation was wrong and was withdrawn in session.**
It was an orthographic overlay layer in target pixels, which would have required
amending ADR-0049's *no orthographic overlay path*. The principal declined it on
a constraint that appears nowhere in the record:

> *"Having it in 3D was a good choice. this way we can have text materials and
> text shaders, but we are also prepared for VR."*

That argument is decisive and is the reason this ADR exists in the shape it does.
An orthographic panel has no position in space, so there is nothing to hand the
two eyes: head-locked and drawn orthographically, it has no stereo separation and
reads as an object at infinity that is nonetheless in front of your face. Every
headset platform's guidance says the same thing — put the panel in space, at a
distance in metres. Even the compositor-level layers headsets provide, the
closest thing to an overlay that exists there, are quads with a pose and a size
in metres. So an orthographic layer is not a shortcut that later generalises: it
is work thrown away, and in the meantime it is the *easy* answer, so every HUD
written before the headset arrives is written in the one form that cannot
survive. That is the failure ADR-0049 exists to prevent, applied to the tech
lead's own proposal.

**Card 021b's coder reported this before the principal did, refused to work around
it, and named the missing axis correctly.** Its *The heads-up line is hidden by
geometry* section establishes that the behaviour is the card working as written
rather than a defect; that the mechanism is ADR-0061's blended pipeline testing
depth without writing it, so any nearer opaque surface wins; that *"always on
top" and "screen space" are two different things and only one of them is
refused*; and that what the engine lacks is **a layer** — *"the alpha mode says
how a fragment blends and `3d/depth_sort` says in what order the blended ones go,
and nothing at all says whether an object belongs to the world or to something
drawn over the top of it."* It listed shapes, declined to recommend one, and sent
it here. That is the escalation working, and Option E below and the argument that
kills it are both the coder's.

**What made the question decidable was separating two things the options had run
together.** *Ordering* — how overlay content stops being penetrated by geometry
near the camera — and *projection* — orthographic in pixels, or perspective in
metres. They are independent. VR settles projection and says nothing about
ordering, and ordering is still a real problem in a headset: a head-locked panel
at two metres is penetrated by a wall at one metre there too.

Constraints already fixed:

- **Everything is in 3D space** (ADR-0049): one renderer, one shader; `text`,
  `sprite` and `ui` produce meshes and textures and get no path of their own to
  the framebuffer; a GUI on a surface in the world is the case being protected.
- **Reverse-Z, depth 0..1, near at 1.0, `GREATER`, cleared to 0** (ADR-0033),
  chosen for the depth precision it buys. Its §5 names an orthographic
  convention; **no orthographic projection function exists in the engine today**,
  and this ADR does not add one.
- **The frame is an offscreen colour and depth target, copied to the window**
  (ADR-0051), per frame slot (ADR-0050).
- **The blended pass** (ADR-0061): one later pass into the same target, depth
  test on, depth write off, sorted back-to-front per object on the CPU in `3d`.
- **Unlit is a material property, not a second shader** (ADR-0071), and the rule
  it established: *"this surface draws differently"* is answered with a material
  property until a branch is shown to be the wrong tool.
- **Exactly one camera, and more than one is a bug rather than a choice**
  (`3d/draw_system.h`), which explicitly defers a second camera to *"the card
  that introduces a viewport"*.
- **A capability priced by content is never a setting** (ADR-0072); **nothing
  costly is on unless the program asks** (ADR-0066); **implement on demand**
  (ADR-0034) and **no premature generality** (ADR-0008).

## Options considered

### Option A — reserve the near end of the depth range
Overlay content draws in a second group with the viewport's depth range squeezed
into a sliver at the near end, so it always wins the depth test. Quake's
viewmodel trick, two lines of dynamic state, no new concept anywhere. It fixes
occlusion and nothing else: the HUD stays a world object with its thirty lines of
placement and its jitter, its size still depends on the field of view, and the
world permanently loses the part of the range the sliver took — which is the
precision reverse-Z was chosen to gain.

### Option B — an orthographic overlay layer in target pixels
The conventional answer, and the tech lead's first recommendation. Positions are
pixels, resize handles itself, the placement code disappears. It requires
amending ADR-0049, and it is the one form a headset cannot present.

### Option C — a layer above the world, drawn in the same perspective camera, with depth cleared between them
The world is drawn as today; depth is then cleared and overlay content is drawn
into the same target through the same camera. Same shader, same materials, same
alpha modes, same unlit flag. Overlay content keeps a real position in metres, so
it has stereo parallax the day a headset exists, and it stops being penetrated
today. Costs a second rendering block and a per-entity layer.

### Option D — a general layered-camera system
Any number of layers; cameras declare which layers they draw, in what order, with
what projection and into what target. Unity, Godot and Bevy all arrive here.
Probably where this engine eventually goes, and taking it now means building
multi-camera and second-target support — the card `3d/draw_system.h` explicitly
defers — to serve one heads-up line.

### Option E — a third pipeline with the depth test off
Card 021b's first shape, and the smallest thing that works: one pipeline beside
the two card 021a built, blend on, **depth test off**, drawn after everything,
with some way for an entity to say it belongs there. Nothing else in the engine
changes.

**Its own report names the catch that decides against it.** Turning the depth
test off does not only stop the scene occluding the layer — *"it stops the
layer's own objects occluding each other. One line of writing does not care. A
GUI of overlapping panels does, and it would be back to relying on the order the
draws are issued in."* So it solves the heads-up line and hands card 023 a layer
its panels cannot sort inside. The report's own conclusion is that 023 wants the
cleared-depth shape and that **picking this one now is picking it for 023 as
well**.

## Decision

**Option C**, with the placement half fixed at the same time.

The deciding factor, named by the principal: **an orthographic layer is the one
answer that has to be thrown away in a headset, and it would be the easy default
until then.** Option A fixes the symptom; Option C removes the cause and costs
about the same.

1. **Overlay-ness is a property of the drawable, and both placements are
   first-class.** A GUI panel that hangs in the world, is walked behind and is
   penetrated like anything else, and a GUI panel that is always in front of the
   world, are both available to a program, and **neither is the default the other
   opts out of.** This is the principal's requirement stated in his own terms and
   it is the load-bearing half: the moment one of them is the exception, the
   other becomes second-class, which is the failure ADR-0049 was written against.

2. **The overlay layer is drawn in the same perspective camera, and its positions
   are positions in metres.** No orthographic projection is introduced anywhere in
   the engine by this decision. Overlay content is geometry in the same space as
   everything else; the only thing the layer changes is what it is ordered
   against.

3. **Ordering is achieved by clearing depth between the world and the overlay,
   not by splitting the depth range.** The split is the cheaper-looking answer and
   it taxes world depth precision permanently, in exactly the currency ADR-0033
   spent a decision to acquire. A clear costs nothing and takes nothing away.

4. **Inside the layer, nothing is different.** The same shader, the same material
   records, the same three alpha modes, and ADR-0061's blended rules apply within
   the overlay as they do within the world.

5. **The layer decides order and nothing else. Shading stays on the material.**
   An overlay element that does want the sun on it is simply a material that is
   not unlit. Putting *unlit* on the layer as well would give the engine two
   places that decide whether something is lit, and ADR-0071 rejected a second
   such place on exactly this ground. The principal's confirmation:
   *"You are right that unlit is enough for ui layers."*

6. **Head-locking is the engine's job, not the program's.** An entity says it is
   a fixed offset in front of the camera and the engine resolves that after the
   camera has moved. This is where the ergonomics the tech lead wanted actually
   live — not in the projection at all — and it is the correct primitive under its
   real name. It removes `facing_the_camera`, its thirty lines and its jitter
   comment.

7. **Two layers is the cut: world and overlay.** A third is a decision, not a
   parameter, and Option D is what it grows into if it is ever wanted.

**Not decided here:** how overlay elements order against each other (D-099),
where the overlay pass sits once post-processing exists (D-100), and how a
camera-relative transform is spelled and where it is resolved (D-101).

## VR, and exactly what standing it has

Stereoscopic rendering **is not planned**, in the principal's words *"not planned
for. but might be anytime."* It is not on the capability list, it is not a
commitment, and it buys no work: nothing is built, abstracted or made general for
it, and a card that costs real effort today for a headset that may never come is
refused the same as any other speculative work under ADR-0008.

**What it does is break ties.** Where two answers cost about the same and one of
them would have to be thrown away in a headset, take the other. That is precisely
what happened here — Option B and Option C are within a card of each other in
effort — and it is the whole of the standing this gets. Recorded because it just
changed a decision, and under this project's rule that the repository is the only
memory, an argument that decides something and is not written down is an argument
the next session has to have again.

## Blast radius

**Moderate, and mostly in what it prevents.**

Reversibility: **moderate.** Adding an orthographic projection later is a small
function and a second projection path — cheap in code. What would not be cheap is
the content: every HUD, panel and readout written against an orthographic layer
between now and then is authored in pixels, and pixels do not convert into a
position in metres by any mechanical rule. **The expensive direction is the one
not taken**, which is the point.

The part to guard is **point 1**. If overlay ever becomes the default that world
placement opts out of, GUI-in-the-world quietly becomes the unusual case, and the
engine ends up where every engine with a screen-space canvas ends up, without
anyone deciding to go there.

## Consequences

- **ADR-0049 survives intact and unamended**, and it was strengthened by the
  principal against the tech lead rather than the other way round. That is worth
  recording plainly: the reading pinned a year of decisions ago held up the first
  time something real pushed on it.
- **The one-camera assert stays true**, no second camera exists, no second target
  is created, and the viewport card `3d/draw_system.h` defers is not pulled
  forward. Option D would have done all four.
- **The draw becomes world-opaque, world-blended, depth clear, then the overlay's
  own opaque and blended.** Up to four groups where there are two, and the second
  rendering block loads colour and clears depth. Whether the overlay needs both
  of its groups from the start is the card's to answer — text is blended, so the
  blended one is not optional.
- **A program with no overlay content pays for nothing**, which is ADR-0072's
  test passed: the cost is priced by content, so this is never a setting.
- **Drawing over a wall is correct on a monitor and is a comfort question in a
  headset**, where occlusion would say the panel is nearer and stereo would say
  the wall is. VR practice mostly places UI so it is not penetrated rather than
  drawing through things. Named so it is a known edge for whoever writes that
  card, and explicitly not solved here.
- **`facing_the_camera` in the dev program becomes dead code** when point 6
  lands, along with the comment explaining why it sits between two systems. The
  comment is worth keeping somewhere: it is the clearest statement in the tree of
  why intent ordering within a frame matters (ADR-0065).
- **Head-locked placement constrains loop order** — it has to resolve after the
  camera system and before the transform system drains, which is the gap the dev
  program found by hand. That is an argument for it being a system rather than a
  component read at draw time.
- **No orthographic function is added**, so ADR-0033 §5's orthographic depth
  convention stays written down and unused. It remains correct for whatever
  actually wants it first, which is more likely to be shadow cascades than UI.
- **The statistics readout is still not on screen.** D-092 is untouched by this:
  ordering was never why it could not be drawn.
- **Card 023 is unblocked in the part that was blocking it**, and it is the
  principal's to rewrite. The tech lead's licence to write cards was given for
  one split on 2026-09-05 and was explicitly not standing.

## Rejected options and why

- **Option A — the depth-range split.** The closest call, and the principal's own
  first instinct. Rejected on two counts that only appeared once the options were
  separated: it spends reverse-Z's precision, permanently, to buy an ordering that
  a depth clear gives for free; and it leaves the placement half — the thirty
  lines and the jitter — untouched, so the problem would come back under a
  different name on the GUI card. It is not wrong, it is half.
- **Option B — the orthographic overlay.** The tech lead's recommendation,
  withdrawn in session. Rejected because it is the single answer a headset cannot
  present, because it would have become the default before that was discovered,
  and because the ergonomics argument for it turned out to belong to head-locking
  rather than to the projection — so its whole case was recoverable without it.
- **Option E — a third pipeline with the depth test off.** Rejected on the catch
  its own author found: it buys ordering against the world by giving up ordering
  *within* the layer, which is free with a depth clear and is exactly what a GUI
  of overlapping panels needs. It is the cheapest option on this list and it is
  cheap in the way that gets paid for twice.
- **Option D — general layered cameras.** Rejected as generality with one
  consumer (ADR-0008, ADR-0034). It forces multi-camera and a second target — a
  deliberately deferred card — to place one line of text. It stays the shape this
  grows into if a third layer is ever genuinely wanted.

## Questions this opens

- **D-099** — **how overlay elements order against each other.** Depth is cleared,
  so everything in the layer passes the test against the world; within the layer,
  ordering is either the blended pass's existing back-to-front sort by distance —
  which works, because overlay elements have real positions in metres — or an
  explicit order number, which is what a UI author usually wants and which the
  engine has nowhere to put today. Trigger: the first overlay with two elements
  that overlap.
- **D-100** — **where the overlay pass sits once post-processing exists.** A HUD
  that is bloomed, tone-mapped and motion-blurred with the world is usually wrong,
  which argues for drawing it after the post chain; but that means the overlay
  writes into a target that has already been tone-mapped, which changes what
  colour space its materials are authored in. Both halves are real. Trigger: the
  first post-process, which is also when ADR-0051's blit-versus-full-screen-quad
  question comes back.
- **D-101** — **how a camera-relative transform is expressed, and where it is
  resolved.** A component holding an offset and resolved by a system between the
  camera and the transform drain, or a flag on the transform interpreted at draw
  time. The dev program's `facing_the_camera` comment is the evidence for the
  first. Trigger: the card that implements point 6.
