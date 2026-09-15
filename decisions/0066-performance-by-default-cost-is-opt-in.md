# 0066. Performance by default: nothing costly runs unless asked, and the engine never hides what it costs

- **Status:** Accepted
- **Date:** 2026-09-05
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

The principal stated the engine's performance philosophy directly, while deciding
present mode:

> Performance by default. If you want caching or throttle draws to 60fps or
> whatever, you turn that on or write it yourself. Our engine (rendering,
> physics, you name it) goes full blast. Then it is up to the developer to add
> stuff that slows it down. Which is why we will later have no post processing
> effects or realtime GI or any other slow stuff on at startup. Just raw
> performance with as little as possible turned on.

This is a standing rule, not a decision about one feature, and it is written down
now because it pre-answers a long queue of arguments that would otherwise be had
one card at a time — does ambient occlusion ship on, does the engine cache, does
it throttle, does it pick a quality preset for the developer.

**It also needed sharpening before it could be applied**, which is what writing
it exposed. The principal asked whether MAILBOX suits the philosophy. It does not
— MAILBOX draws roughly fifteen frames for every one anybody sees — and yet the
instinct behind the question was sound. Two different principles were sitting in
one sentence:

| | Principle | What it forbids |
|---|---|---|
| **Cost** | Nothing costly runs unless asked | Unrequested spending — post-processing, GI, caches, throttles |
| **Visibility** | The engine never hides what it costs | A default that makes speed unmeasurable |

They are stated in one ADR because the first is unfalsifiable without the second.
An engine cannot claim to be fast by default if its default configuration makes
its own speed invisible.

**Constraints already fixed.** ADR-0063 froze capability in named tiers and
requires every effect to ship its baseline path first. ADR-0064 made settings a
request clamped to the tier, with no presets and no persistence in the engine.
ADR-0025 requires a per-frame rendering cost to be argued in advance. This ADR is
the principle those three were already applying without having named it.

## Options considered

### Option A — performance by default, cost is opt-in

The engine starts in its cheapest correct configuration. Every feature with a
per-frame cost is off until a program turns it on. Bevy is roughly here by
construction; a bare `bevy_pbr` app has no post-processing stack until plugins
add one.

### Option B — good-looking defaults

Ship the configuration that makes a new project look its best: tone mapping,
antialiasing, ambient occlusion, bloom. Unreal's shape — a new project is
visually rich and considerably slower than an empty renderer, and much of
optimisation work on that engine begins by switching things off.

### Option C — quality presets

Decide nothing; ship low/medium/high and let the developer pick a bucket.

## Decision

**Option A, the principal's stated philosophy, adopted as a standing rule.**

**1. Nothing with a per-frame cost is on at startup.** Post-processing, realtime
global illumination, ambient occlusion, antialiasing beyond none, shadow
techniques beyond the baseline, temporal anything — each ships off and is turned
on by the program that wants it. A card proposing a feature that is on by default
is contradicting this ADR and needs one that supersedes it.

**2. The engine does not spend on the developer's behalf.** No implicit caching,
no automatic level of detail, no frame-rate throttling, no background work the
program did not ask for. If a program wants any of it, it turns it on or writes
it.

**3. The engine never hides what it costs.** Measurement is cheap and available,
and the development picture is the honest one. This is the half that makes rule 1
checkable, and it is why the dev program's timing block is a permanent feature
rather than card 020's scaffolding.

**4. The boundary: this forbids unrequested *cost*, not convenience and not
safety.** It is not an argument against a feature being easy to switch on, nor
against a cheap guard that prevents a catastrophe — `MAX_FRAME_SECONDS`, the
quarter-second clamp that stops a stalled frame teleporting the world, stays and
is not in tension with this. The test is whether the program asked for the
spending, not whether the code is simple.

**5. Defaults are the engine's; a program overrides them.** Under ADR-0064 a
setting is a request clamped to the tier, so "off by default" never means
"unavailable". `render` and the other engine folders carry no knowledge of which
*kind* of build is running — a program that wants more asks for more, and the dev
program and the editor are programs like any other.

## Blast radius

**Load-bearing, and deliberately so.** This is the ADR later cards are measured
against, and its cost is paid in refusals: every future effect arrives with an
argument for why it may be on, and the answer is no unless this ADR is superseded.

Reversing it is not a code change but a reputational one — an engine that starts
cheap and one that starts pretty attract different users and are benchmarked
differently. Cheap to reverse per feature (flip a default), expensive to reverse
as a philosophy, which is the definition of a decision worth writing down.

## Consequences

- **The engine looks plainer out of the box than its competitors.** A screenshot
  of a new project will be less impressive than the equivalent in an engine that
  ships a post stack. This is a real cost, it is accepted, and it should not be
  relitigated one effect at a time by anyone who finds the comparison unflattering.
- **Every expensive feature needs a documented way to be turned on**, which is
  more public surface than a preset would be. ADR-0064's knobs are that surface.
- **Off by default is a path nobody tests.** This is the dangerous consequence and
  it is the same failure ADR-0063 rule 4 named: a path nobody can run is not
  shipped. The dev program must therefore *deliberately* exercise expensive paths
  even though they are off by default, and that is a standing requirement on
  whoever adds one, not an optional extra.
- **"Performance by default" is quotable and elastic.** Rule 4 exists because a
  principle this appealing will otherwise be used to argue against convenience,
  ergonomics and safety, none of which it says anything about.
- **The philosophy has a marketing shape**, and that is a legitimate part of it:
  an engine whose first number is its real number is a different product from one
  whose first number is a preset.

## Rejected options and why

- **Option B, good-looking defaults** — rejected. Every default cost is paid by
  every game whether or not it wanted it, and it is paid in the place hardest to
  recover: the developer's first impression of how fast the engine is. It also
  makes the engine's own performance work unmeasurable, because the baseline is
  never the baseline.
- **Option C, quality presets** — rejected, and already rejected once by ADR-0064,
  which explicitly ships no presets. A preset is a decision the engine takes on
  the developer's behalf, bundled so that no individual cost is visible. That is
  the opposite of rule 3.

## Questions this opens

- **D-085** — What counts as "costly", stated well enough to settle an argument. A
  threshold in a fraction of a frame, a category list, or a case-by-case judgement
  recorded per feature. Trigger: the first card whose feature is arguably free.
- **D-086** — Where the always-available measurement lives once `app` exists. It is
  in `dev/src/main.c` today, which means it is the dev program's and not the
  engine's, so rule 3 currently holds by accident. Relates to D-081.
- **D-087** — Whether the *editor* is bound by this philosophy or is a program that
  turns things on. ADR-0057 makes it a C program on the engine, so it may simply
  opt in — but an editor whose viewport looks nothing like a shipped game is its own
  problem. Trigger: the first editor rendering card.
