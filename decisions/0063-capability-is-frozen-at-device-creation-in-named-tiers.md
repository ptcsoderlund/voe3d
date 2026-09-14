# 0063. GPU capability is frozen at device creation, in named tiers, and every effect ships its baseline path first

- **Status:** Accepted
- **Date:** 2026-09-04
- **Deciders:** Human, Tech Lead

## Context

The principal's question: if we do raytraced shadows, what happens on a graphics
card with no raytracing?

**Half of it is already answered and implemented.** `render/src/device.c` sets
Vulkan 1.3 as a hard floor — a card below it is skipped during selection, and if
none qualifies the engine says which of the two things is missing and refuses to
run. Dynamic rendering and `synchronization2` are core in 1.3 and the renderer is
built on both, so there is nothing to fall back to. That is the pattern for a
**required** feature and it needs no change.

The open half is the **optional** feature: one where the engine could run either
way, which is what raytracing would be.

Constraints already fixed:

- **No runtime plugin boundary** (ADR-0008), so this is not solved with
  swappable backends, and the RHI-with-one-backend shape was rejected there too.
- **Shaders are compiled offline and embedded** (ADR-0046). Every capability axis
  multiplies the shader set baked into the binary. This is the cost centre, not
  the C code.
- **Recoverable failure is a returned value with a message** (ADR-0041). Refusing
  a machine is a returned failure, never an assert.
- **Geometry lives in bump-allocated pools that never free** (ADR-0059), with
  freeing and compaction already open (D-068).
- **Implement on demand** (ADR-0034) and **GPU work is justified, not default**
  (ADR-0020).
- **The principal is the only Windows tester**, always — a standing property, not
  a gap.
- **v1 has no shadows at all**, and raytracing is not on the *later* capability
  list either. Nothing on the board consumes this today.

## Options considered

### Option A — floor only; no optional features ever
If a feature matters, raise the floor and refuse below it. One code path, one
configuration, an honest message. Raytracing would become a system requirement of
every game built on the engine, excluding integrated graphics and anything
pre-2018.

### Option B — a small number of named tiers, frozen at device creation
Capability is discovered once during device selection and frozen into one of a
few complete configurations. A feature belongs to a tier; each tier is a
configuration somebody actually runs. Unreal's feature levels and Unity's
graphics tiers are this. Costs a shader variant axis and the discipline to keep
the tier count tiny.

### Option C — per-feature capability flags with independent fallbacks
A `caps` struct of booleans and a check at each site that cares. Maximum hardware
reach. Ten independent flags is a thousand configurations of which one is tested,
and the untested ones fail only on hardware nobody here owns.

## Decision

**Option B**, and the content is four rules rather than a tier list.

1. **A feature is required or it belongs to a tier. There is no third kind.**
   Required means the floor moves and the refusal message names what is missing
   — the 1.3 path that already exists. Anything else is tier membership.
2. **Capability is discovered once, in device selection, and frozen.** It is
   `render`'s alone: nothing outside `render` queries the hardware, and what
   leaves `render` is a **tier**, not a bag of booleans. No mid-frame queries and
   no per-call-site checks.
3. **Every effect ships its baseline path first.** An effect enters the engine
   with the implementation that runs on the baseline tier. Shadows means shadow
   maps before raytraced shadows. A top-tier-only effect is possible but needs
   its own ADR arguing why the baseline is not served.
4. **A path nobody can run is not shipped.** Two paths means both are run —
   Linux by the coder, Windows by the principal. If no one can run a tier, it is
   not added; an untestable tier is worse than an absent one because it produces
   bug reports that cannot be reproduced.

**The tier list today has exactly one entry: `baseline`** — Vulkan 1.3 plus what
device selection already requires. **No second tier is created by this ADR.** The
card that brings the first feature needing one adds it, by name, with its
membership, in its own ADR.

**Shader variants: one capability axis, and the tier is it.** Under ADR-0046 the
tier is a compile-time variant of the embedded shader set, enumerated in one
place — the cook step is the natural home, as already noted for material variants
(D-062). A second capability axis needs an argument, not a commit.

**Not decided here:** user-facing quality or scalability settings, which are a
different thing from hardware tiers and belong to whatever card first wants a
settings surface. A tier is what the hardware can do, not what the player asked
for.

## Blast radius

**Cheap today, load-bearing in one specific way.** There is no machinery to
build: the rules cost nothing, and `baseline` is a name in a struct that device
selection already fills in. What hardens is **the shape of what call sites read**
— a tier, not flags. Going from flags to tiers later means revisiting every check
in the renderer; going from one tier to two is additive. That asymmetry is the
whole reason to write this before the first check exists rather than after.

## Consequences

- **Hardware reach is bounded by the floor, deliberately.** Cards below Vulkan
  1.3 are refused with a message. That is already true and now it is on purpose.
- **Tiers are coarse and will sometimes be wrong for a specific card.** A card
  with raytracing support but bad raytracing performance gets the top tier
  anyway. Accepted: the alternative is Option C's matrix, and per-card
  performance profiles are a thing this project has no way to measure.
- **Effects arrive in the less exciting order.** Baseline first means shadow maps
  before raytraced shadows, and the shadow-map path stays the one most players
  see. This is a real cost in enthusiasm and the right order anyway.
- **A one-entry tier list will look like premature machinery.** It is not
  machinery — it is a rule and a name. If a reader wants to delete it, the thing
  to delete is the second tier that was added without an ADR.
- **Raytracing, when it comes, pulls on the geometry pools.** Acceleration
  structures are built from the same vertex and index pools that never free, and
  they need rebuilding when geometry changes. That makes pool freeing and
  compaction materially more urgent than it is today.
- **The editor inherits a question**: how it previews a tier it is not running
  on. Not answered here.
- **`render`'s public surface gains one small thing** — the tier it selected —
  added under ADR-0060 by its first caller, not now.

## Rejected options and why

- **Option A — floor only.** Clean, and it makes raytracing a system requirement
  for every game built on the engine. For a general-purpose engine whose promise
  is that a programmer clones it and builds, requiring current hardware for a
  feature that has a perfectly good baseline implementation is the wrong trade.
  It stays the right answer for anything with no baseline, which is how Vulkan
  1.3 itself is treated.
- **Option C — per-feature flags.** The default outcome if nobody decides, because
  each individual check looks harmless. Rejected on testability: the
  configuration count is exponential in the flag count, the tested fraction goes
  to zero, and the failures land on hardware nobody here owns. It is also the
  shape that scatters hardware knowledge through the renderer, which rule 2
  exists to prevent.

## Questions this opens

- **D-076** — acceleration structures over the geometry pools, and what
  raytracing does to pool freeing and compaction (D-068). Trigger: the first
  raytracing card.
- **D-077** — whether user-facing quality settings exist at all, and how they
  relate to a hardware tier. Deferred; trigger: the first card wanting a settings
  surface.
- **D-078** — how the editor previews a tier it is not running on, given the
  editor is a C program on the engine (ADR-0057). Trigger: the first editor
  rendering card.
