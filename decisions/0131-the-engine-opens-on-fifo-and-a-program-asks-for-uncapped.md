# 0131. The engine opens on FIFO, and a program asks for uncapped

- **Status:** Accepted
- **Date:** 2026-09-11
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

**Closes D-216.** ADR-0067 rule 1 says `render` opens on FIFO and rule 2 says uncapped is
not a second default but a request a program makes at startup; that ADR's own consequences
name the follow-up as *the dev program's startup changes*. **The code does the opposite.**
`render/src/device.c` sets `present_wanted = VOE_RENDER_PRESENT_MAILBOX` at device
creation, and `dev` makes no startup request at all — it inherits it. `render/src/swapchain.c`
documents the drift as deliberate and attributes it to the principal, but no ADR supersedes
ADR-0067, and this repository's fourth hard rule is that a decision is not made until its
ADR exists. Found 2026-09-10 while investigating bug 003.

The principal's position, 2026-09-11: *"I think the standard way engines work is to not have
v-sync?"* — and, on being shown that it is not: *"it seems important for you to do fifo by
default, go ahead and make that decision. You are the tech-lead after all."* **Delegated.**

**The facts that decided it**, because the premise is the whole argument:

- **Mailbox is not v-sync off.** It does not block the calling thread, but the display still
  shows frames only at its refresh rate, so it does not tear — it renders frames and
  discards them. True v-sync off is `IMMEDIATE`, which tears and which this engine does not
  use. The high frame counter measures work, not pictures.
- **Shipping games default v-sync on.** Unreal, Unity and Godot all enable it in the player
  build. What runs uncapped is the *editor viewport* and dev and benchmark builds, where the
  point is to see real cost. That is precisely the split ADR-0067 made.
- **Uncapped costs a shipped game.** `IMMEDIATE` tears, which reads as broken. Mailbox does
  not tear but runs the GPU flat out — rendering three or four frames per frame displayed is
  that multiple of the power, for nothing anyone can see. On a laptop it is battery and fan
  noise.
- **FIFO is the only present mode the Vulkan specification requires every implementation to
  support.** Defaulting to the guaranteed one and requesting the optional one is the
  direction with no fallback path in the common case.

## Options considered

### Option A — restore ADR-0067
`render` opens on FIFO; a program that wants uncapped asks at startup; `dev` asks.

### Option B — supersede ADR-0067 and make mailbox the engine's answer
Write down what the code already does. Defensible: tear-free, lower latency than FIFO, and
for an engine whose first users are developers the number on screen is honest.

## Decision

**Option A.** ADR-0067 is restored to force; it is not superseded and not amended.

1. **`render` opens on FIFO.** The device does not set a wanted present mode at creation.
2. **A program that wants uncapped asks for it at startup.** `dev` asks, because it is a
   development tool and its whole purpose is showing what the engine can currently do.
3. **The `P` key keeps working exactly as it does**, swapping the two modes at runtime.
4. **The comment in `render/src/swapchain.c` attributing the current default to the
   principal is removed**, because it is the only record of a decision that was never made
   and it would otherwise outlive this one.

## Blast radius

**Cheap.** One line out of `render`, one line into `dev`, one comment deleted. Reversing it
is the same three edits. Reversibility: **cheap.**

## Consequences

- **`dev`'s numbers do not change**, because `dev` asks for mailbox. Anyone reading the
  readout sees what they saw yesterday.
- **A program that asks for nothing now gets FIFO**, which is the change, and it is the
  change that matters: it is what a game built on this engine gets before anyone thinks
  about it.
- **The Windows frame-rate report (bug 003) is unaffected and stays parked.** The principal
  parked it — *"its not a problem so we leave it until it is"* — and this ADR does not
  revive it. It does mean a shipped game on a 120 Hz Windows display would now see 60 rather
  than the compositor's rate, which is the half of that report worth watching, and the
  trigger written into it already covers it.
- **A game will eventually want to expose this to players**, because some genuinely want
  uncapped for input latency and that is their call. Recorded as D-241; nothing needs it
  until there is a game with a settings screen.

## Rejected options and why

**B — mailbox as the engine's answer.** Rejected, though it was a real option and the tech
lead offered to write it. It optimises for the developer looking at a frame counter, which
is a `dev` concern that `dev` can state for itself, at the cost of every shipped game
burning power to render frames nobody sees. The engine's default should be what a game
wants; the tool's default is the tool's to ask for.

## Questions this opens

- **D-241** — how a game exposes the present mode to a player, and where that setting lives.
