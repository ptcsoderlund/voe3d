# 0083. A module produces things to draw; a switch changes how drawing happens — and the core is built to accept switches

- **Status:** Accepted
- **Date:** 2026-09-06
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Amended 2026-09-06, same session:** the principal sharpened the test on
  reading this. *"If it has to be in engine, it should be a switch. Meaning stuff
  that would require modifying the engine. Everything else can be in modules."*
  **No part of the decision below changed** — every example classifies identically
  — but the test is now stated by its consequence rather than by its category, and
  the *purpose* of a switch is corrected. See §1a, which is the amendment; §1 is
  kept because it is still the structural explanation of *why* the two classes
  differ.
- **Amended again 2026-09-06, same session:** the principal, on the standing risk
  of over-planning — *"we cannot let perfection stand in the way of progress. How
  can we make switches and core features before we know what we want and need? So
  yeah, we introduce core features when we need them. We cant cover all cases in
  beforehand."* **§3's first bullet was the one place this ADR contradicted its own
  guard**, asking for switchable values to be consolidated in advance, and it is
  corrected: the core owes nothing today and a switch consolidates what it needs
  when it arrives. §3's third bullet gains the same point about §4's list. **This
  closes D-116**, which had been left open on exactly this question. Nothing else
  changed; the rest of the section already said it.

## Context

The principal restated the engine's direction, in answer to a question about
whether the recent run of *decisions not to build* was painting us into a corner:

> *"I do want a general purpose engine. Meaning the dev should be able to decide
> in an editor what they want and how they want it. Which is why I want as much as
> possible as \"opt-in\" instead of \"opt-out\". So we do the renderer as basic as
> it gets. Then we do stuff as features or modules to opt-in on within the editor
> (or extend it yourself)."*

And, on the shape of a core feature: *"I want shadowmaps", "Switch it on"*.

**Most of this is already written and binding**, which is worth saying plainly
because it means the direction is not a change of course:

- **ADR-0066** is this philosophy already adopted as a standing rule: nothing with
  a per-frame cost is on at startup; post-processing, ambient occlusion,
  anti-aliasing, temporal anything each ship off and are turned on by the program;
  a card proposing a feature on by default is contradicting a decision and must
  say so.
- **ADR-0064** is the mechanism: a setting is a request clamped to the tier, every
  knob declares its cost class (free, or a rebuild that may drop a frame), the
  engine ships no settings UI because menus belong to the game and the editor, and
  **the knob list is deliberately empty — each feature brings its own.**
- **ADR-0022 and ADR-0030** already shape the folder map the way the direction
  wants: `text`, `sprite` and `ui` sit beside `3d`, each on `render`, each
  producing geometry and materials.
- **ADR-0008** refused a runtime plugin boundary, by name, as premature
  generality — on the grounds that VOE3D has no plugin author. Its cost note says
  adding an indirection boundary later is a change of mechanism at one seam and
  not a rewrite, because every folder already configures standalone.
- **ADR-0051** put the frame in an offscreen image, which is the seam every
  post-process needs and is already built.
- **ADR-0020, ADR-0055** forbid generality with nothing to measure against.

**What is genuinely new, and what this ADR is for:** the direction says *features
or modules to opt in to*, and there is a class of capability that **cannot be a
module no matter how it is designed**. If that is not written down, the promise
breaks the first time someone tries to keep it.

**And it is already half-foreclosed in the code.** The frame's sample count is
written independently in `render/src/target.c:89` and `render/src/device.c:772`
(and in `probe.c:82`), and the swapchain format in `device.c:98`. Nothing is
wrong with any of them; the point is that they were each written once, locally,
and a feature that needs them to agree has to find all of them first.

## The distinction

**Some capabilities change the shape of the frame, and nothing outside the
renderer can add them.**

- **Anti-aliasing.** The sample count is fixed when an image is created and
  **every pipeline must declare a matching one.** An external module cannot make
  the engine's own pipelines multisampled.
- **Tone mapping and wide colour.** Changes the format of the image the frame is
  drawn into, which every shader writing to it has to agree about.
- **A post-process chain.** Changes what the frame is drawn into, and how many
  times it is read and written.
- **Shadows.** Every lit shader either samples a shadow map or does not. That is a
  variant of the core shader, not an attachment to it.

Against those, the things that genuinely are modules — sprites, text, GUI,
particles, importers, animation, physics — all do the same thing: they **produce
geometry and materials and hand them to the renderer.** They need nothing of the
renderer's internals, which is why the folder map could already place three of
them before any existed.

## Options considered

### Option A — write the line down, and name the switches the core must not foreclose
State the test, and list the small number of core switches visible today so the
renderer is built without accidentally making them expensive. Build none of them.
Costs: one rule, and a mild constraint on the basic renderer for features nobody
has asked for.

### Option B — leave it, and decide per feature when each arrives
Costs nothing today. The risk is precise: the first switch-shaped feature arrives,
finds a renderer with nowhere to put it, and gets built as a hack — or worse, gets
argued about as *"but everything is supposed to be a module."*

### Option C — design the extension and module system now
A registration mechanism, a feature interface, hooks. Rejected before it is
argued: ADR-0008 already refused exactly this by name, and nothing has changed —
there is still no plugin author.

## Decision

**Option A.** The deciding factor, in the principal's own framing: **"as basic as
it gets" must mean *nothing is on*, not *nothing can be turned on*** — and the
difference between those two readings is decided by whether the core was built to
accept a switch or has to be reopened to receive one.

### 1. The test, and it is the whole rule

**A module produces things for the renderer to draw. A switch changes how the
renderer draws.**

- **A module** can be added from outside, by us or by a developer, without the
  renderer knowing it exists. It depends on `render`; `render` never depends on
  it. This is the existing folder map and the existing one-way arrows, and this
  ADR adds nothing to it.
- **A switch** cannot. It changes state the renderer's own targets, pipelines or
  shaders are built from, so it lives *in* the core and is turned on from outside.

Every future card answers this question first, and the answer determines where the
code goes. A card that finds itself wanting a hook in `render` so that something
outside can change how drawing happens has found a switch and mis-filed it as a
module.

### 1a. The test, restated by its consequence — and this is the primary one

**Would a developer who wants this have to modify the engine? Then it should be a
switch. Everything else is a module.**

This is the principal's formulation and it is better than §1's for two reasons,
both of which matter more than the tidiness of the categories:

- **It is decidable from outside.** §1 asks what a thing *is* — does it produce
  geometry, or change how drawing happens — which requires knowing the renderer's
  internals to answer. This asks what a thing *costs the person who wants it*,
  which anyone can answer, including the developer asking for it.
- **It corrects what a switch is for, and this ADR had it backwards.** §1 arrives
  at switches as a residue: the unfortunate class of things that *cannot* be
  modules. That framing makes the switch list a list of limitations. It is the
  opposite. **A switch exists precisely so that nobody has to modify the engine to
  get the thing.** The switch list is the anti-fork list, and every entry on it is
  a fork that did not have to happen.

The two tests agree on every example in §4 and on every module named in §1, which
is why this is an amendment and not a new decision. Where they would differ, **§1a
governs**: the question is always whether the want can be satisfied from outside,
not which side of a taxonomy the feature falls on.

**What this does *not* license, and the distinction is the whole guard.** *Would
someone have to modify the engine* is a question about a real want, not a
hypothetical one. Anything at all could be phrased as something a developer might
one day want to change, and answering that with a switch is how the renderer fills
with hooks nobody uses. **§3's second bullet is unchanged and is what keeps this
honest: no switch exists before the card that builds the feature behind it.** The
trigger for a switch is a feature being built, not a want being imagined.

### 2. A switch is still opt-in, and this does not weaken ADR-0066

**A switch ships off.** It is requested through the settings path ADR-0064
already defines — a request, clamped to the tier, with a declared cost class,
reporting what actually resulted. The principal's shape is exactly right and is
the contract: **"I want shadow maps" → "switch it on."** Nothing here puts a
per-frame cost on by default, and nothing here is an exemption from ADR-0066.

### 3. What the core owes: do not foreclose, and build nothing

This is the part that must not become a licence for the generality ADR-0008 and
ADR-0020 refuse. The obligation is **negative**:

- **A switch consolidates what it needs, when it arrives — nothing is tidied in
  advance.** Corrected by the second amendment; as first written this bullet asked
  for the frame's sample count and colour format to be given one home *now*, which
  is work before a need and is the thing the rest of this section forbids. The core
  owes **nothing** today. When the first switch card lands, part of its job is
  gathering the values it needs into one place, and it will know which ones those
  are because it is building the feature. See D-116, which this closes.
- **No switch is created before the card that builds the feature behind it.** No
  empty enum, no unused parameter, no `if (post_process)` with nothing on the
  other side, no interface with one implementation. **A switch arrives with its
  feature or it does not arrive.**
- **The list below is what we can see today, it is not closed, and it is not a
  plan.** Naming a thing on it is not a commitment to build it, and the list being
  incomplete is expected rather than a defect — **we cannot know the switches
  before we know the features**, and any attempt to enumerate them in advance is
  the same premature generality in a different costume. It exists to show what the
  *shape* of a switch is, by example, and for no other purpose. A switch that is
  not on it is not an exception.

### 4. The switches visible today

| Switch | What it changes | Where it would land |
|---|---|---|
| Sample count | Every colour and depth attachment, and every pipeline's declared samples | Anti-aliasing, currently *not planned* and reversible in one card |
| Frame colour format | The offscreen image, and every shader writing to it | Tone mapping and wide colour, on the *later* list |
| A post-process step over the frame | What the frame is drawn into and how often it is read | The whole image-polish row |
| Shadow sampling in the lit shader | A variant of the core shader | The first shadow card |

## Blast radius

**Cheap, and the cheapest version of a rule that is expensive to omit.** It builds
nothing and forbids building anything. Reversing it is writing a different
paragraph.

What is load-bearing is the negative obligation, and it fails in a way worth
naming: **if this ADR is read as permission to add extension points, it has done
the opposite of what it is for.** The renderer filling up with switches that have
no feature behind them is a worse outcome than the retrofit this ADR prevents,
because a hook with no consumer cannot be tested, cannot be removed, and is
indistinguishable from one that matters. §3's second bullet is the guard and it is
not optional.

Reversibility: **cheap.**

## Consequences

- **Every card now answers one question before it is written**: does this produce
  things to draw, or change how drawing happens? That is a decidable question with
  a structural answer, which is the kind that stops arguments rather than starting
  them.
- **"As basic as it gets" gains a precise meaning** and stops being available as
  an argument against configurability. Nothing is on; things can be turned on.
- **The sample count and the frame format each get one home**, which is a small
  tidy-up with no behaviour change and no card of its own until something needs
  it. Recorded as D-116 so it is not lost and not rushed.
- **A developer extending the engine themselves works today for modules and does
  not for switches**, and that asymmetry is honest rather than a gap: a module is a
  folder that depends on `render`, and every folder already builds standalone. A
  developer who wants a *switch* the engine does not have is modifying the engine,
  and that is true of every engine that does not ship a runtime plugin boundary.
  ADR-0008 is unchanged and is not reopened here.
- **The consequence we like least:** this ADR names four switches and authorises
  none of them, which will read to some future card as a to-do list. It is not one.
  Each of those rows is still an unbuilt feature that has to justify itself on its
  own card, and three of the four are on the *later* list or refused outright.
- **The editor is unaffected today.** It is where switches get flipped, but it
  ships no settings UI from the engine (ADR-0064) and the knob list is still
  empty. Nothing here brings that forward.

## Rejected options and why

- **Option B — decide per feature.** Rejected on the specific failure it invites,
  not on principle: the first switch-shaped feature would arrive at a renderer with
  no home for it, and the argument at that point would be about whether the
  direction meant *everything is a module*, which it does not and cannot. One
  paragraph now removes that argument permanently.
- **Option C — design the module and extension system now.** Rejected for the same
  reason ADR-0008 rejected the API registry, and the reason has not weakened: there
  is no plugin author, live reloading is *later*, and adopting a mechanism without
  its requirement is premature generality. The useful half of the capability
  already exists — every folder configures standalone, so extending the engine is
  adding a folder and linking it.

## Questions this opens

- **D-116** — **whether the switchable values are consolidated now or when the
  first switch needs them.** The sample count is written independently in
  `render/src/target.c:89`, `render/src/device.c:772` and `render/src/probe.c:82`,
  and the swapchain format in `device.c:98`. Consolidating is a small,
  behaviour-free tidy-up with no consumer today, which is exactly the shape
  ADR-0020 says to leave alone — against which the first feature to need it pays
  for finding all of them. Trigger: the first switch card, or a card already
  touching those files.
- **D-117** — **whether a developer can add a *switch* without forking the
  engine**, and if so what the seam is. Out of scope today (ADR-0008, no plugin
  author) and recorded because the principal's *"or extend it yourself"* is
  currently true for modules and false for switches, which is a real asymmetry
  rather than an oversight. Trigger: a third-party author, or shipping a game
  without shipping engine source.
