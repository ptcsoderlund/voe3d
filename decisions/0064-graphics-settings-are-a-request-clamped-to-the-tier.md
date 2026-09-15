# 0064. Graphics settings are a request clamped to the tier, and every knob declares its cost class

- **Status:** Accepted
- **Date:** 2026-09-04
- **Deciders:** Human, Tech Lead
- **Closes:** D-077

## Context

ADR-0063 gave the engine a hardware **tier**: discovered once at device
creation, frozen, and the ceiling on what the machine can do. The principal's
follow-up: developers must also be able to turn capabilities on and off
themselves — graphics settings, as every game has.

**These are two axes and conflating them is the standard failure.** A tier is a
ceiling nobody opts into; a setting is a choice at or below it. Downwards is
always allowed — a developer turning shadows off on hardware that supports them
is a legitimate product decision. Upwards is not: no setting raises the tier.

Constraints already fixed:

- **Changing something means asking its owner, and a request is a datatype**
  (ADR-0011, ADR-0013). A settings change is a write across a folder boundary,
  so it is an intent, not a setter reaching into the renderer.
- **`render` has no `ecs` edge** (ADR-0030). `render` therefore *cannot* consume
  an intent queue. Whatever consumes one is `3d` or `app`.
- **Two frames in flight** (ADR-0050). Anything that recreates a pipeline or a
  target must wait for frames in flight to finish; doing it mid-frame is
  corruption.
- **Shaders are compiled offline and embedded** (ADR-0046), and ADR-0063 allows
  exactly one capability axis. A setting can only ever select among variants
  baked in at build time; a game cannot invent one at runtime.
- **Recoverable failure is a returned value with a message** (ADR-0041).
- **`render` grows by-id functions on demand, driven by a caller** (ADR-0060).
- **On demand** (ADR-0034): v1 has no settings surface and no adjustable
  feature, so nothing on the board consumes this yet.

## Options considered

### Option A — a typed settings request, clamped to the tier, cost class per knob
Game or editor code submits a settings intent; `3d` drains it at a defined point
in the frame loop and calls `render`. Each knob declares its cost class, so the
engine knows whether a change costs nothing or costs a rebuild. Defaults derive
from the tier. No UI, no presets, no files in the engine.

### Option B — fine-grained functions, no settings concept
`render` exposes individual setters and every game assembles its own layer,
including the tier clamp. Less to design; every game reimplements validation, and
nothing structurally prevents a mid-frame change tearing a frame in flight.

### Option C — a full settings system
Named presets, serialization, menu metadata, widgets. A product's options menu,
designed before any product exists.

## Decision

**Option A.** The deciding factor: almost nothing has to be invented — the tier
supplies the ceiling and the intent model supplies a safe apply point. The only
genuinely new obligation is that each knob states what changing it costs.

1. **A setting is a request, not a call.** Game and editor code submit a
   settings intent (ADR-0013). **`3d` drains it at a defined point in the frame
   loop and calls `render`'s functions**, because `render` cannot see an intent
   queue (ADR-0030). `render`'s own functions stay plain and by-id (ADR-0060)
   with a stated precondition — called between frames — asserted in debug
   builds.
2. **Every knob declares a cost class**, and the class is part of its
   declaration, not folklore:
   - **Free** — a value the shader or the loop already reads. Takes effect next
     frame. Resolution scale, shadow distance, a light-count cap.
   - **Rebuild** — needs pipelines recreated, targets resized, or a different
     embedded shader variant. Waits for frames in flight, then rebuilds, and
     **may drop a frame.** Vsync, shadow-map resolution, choosing a technique.
3. **The tier clamps, and the engine never silently differs from what was
   asked.** Every apply reports the state that actually resulted. A value outside
   a knob's range is clamped and reported as clamped; a capability the tier does
   not have is **refused** with a message naming the tier it would need
   (ADR-0041). A menu can therefore always show the truth.
4. **Defaults come from the tier.** A game that sets nothing gets a working
   configuration on any machine that passes the floor. A game may instead **pin**
   every knob explicitly and take a refusal rather than a downgrade — which is
   what a deterministic screenshot test needs.
5. **The engine ships no settings UI, no presets and no settings file.** Menus,
   preset names, persistence and their taste belong to the game and the editor.
   The engine's contract is: declare what is adjustable, validate it, apply it,
   report what happened.

**The knob list is empty today**, deliberately, exactly as ADR-0063 left the tier
list at one entry. Each feature brings its knobs when it arrives. Two knobs are
already visible on the board and both belong to their own cards: **present mode**
— vsync, in every game's menu — from the frame-pacing card (D-054), and
**resolution scale**, which the offscreen frame target (ADR-0051) already makes
possible.

**Not decided here:** a developer-facing debug or override channel separate from
game settings, and where a game's chosen settings are stored.

## Blast radius

**Moderate.** The knob list is additive and costs nothing to grow. What hardens
is the *shape* game code is written against — a request that reports the state
that resulted, rather than setters that either succeed or lie. Replacing that
later means touching game code, the editor and the frame loop point together.
The cost-class rule is the part that is expensive to add late: retrofitting it
means auditing every existing knob for whether it was ever safe to flip.

## Consequences

- **Every feature now owes two things beyond its implementation:** its knobs'
  cost classes and its clamp rule. That is paperwork per feature, and it is what
  prevents a settings menu corrupting a frame.
- **A rebuild-class change can drop a frame**, so a menu that live-previews will
  hitch. Correct, and worth saying in the editor's UI when it exists.
- **Two hops for a settings change** — intent, then `3d`, then `render` — where
  Option B had one. This is the module map's price and it is the map working, not
  a wart: `render` staying blind to `ecs` is what keeps a GPU layer testable
  without a world.
- **`render`'s between-frames precondition is convention plus a debug assert**,
  not a type-system guarantee. Only `3d` and `app` call it, both ours.
- **The same game looks different on two machines** when it leaves defaults
  alone. That is what a tier means; the pin-everything path exists for when it is
  unacceptable.
- **Settings intents give D-031 another consumer.** When intents drain is no
  longer only the frame loop's question — a rebuild-class setting needs a drain
  point that is provably between frames.
- **A setting can never select a shader variant that was not built.** Under
  ADR-0046 and ADR-0063 the variant set is enumerated in the cook step; a
  developer who wants a variant we do not build has to change the build, not the
  settings.

## Rejected options and why

- **Option B — setters, no concept.** Rejected on two counts: it pushes the tier
  clamp into every game, and it offers no place to state a cost class, so the
  difference between a free flip and a pipeline rebuild becomes tribal knowledge
  that a menu discovers by tearing a frame.
- **Option C — presets and a settings system.** Preset names are product taste
  and there is no product. Serialization belongs to whoever owns a file format —
  the game, or the editor's project file — not to the renderer.
- **Silent clamping.** The tempting middle: accept anything, quietly do what the
  hardware allows. Rejected because a menu then displays a state the engine is
  not in, and the bug report says "shadows are off even though the setting says
  high".

## Questions this opens

- **D-079** — whether a developer-facing debug/override channel exists separate
  from game settings (environment variable, dev-program keys, a console).
  Deferred; trigger: the first dev program or editor wanting a toggle that is not
  a shipped game setting.
- **D-080** — where a game's chosen settings are persisted, if the engine says
  nothing. Deferred; trigger: the first card that wants settings to survive a
  restart.
- **D-078 gains a half**: the editor previewing a tier it is not running on is
  now also the editor previewing *settings* it is not running under.
