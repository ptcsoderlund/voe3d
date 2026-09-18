# 0067. FIFO is the engine's default present mode; the dev program and the editor ask for uncapped

- **Status:** Accepted
- **Date:** 2026-09-05
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

Card 020 built the frame loop, implemented both present modes behind
`voe_render_present_set/_get`, measured them back to back and — correctly —
refused to choose, because ADR-0025 requires a per-frame cost to be argued and
the card's job was to produce the argument, not to settle it. This is that
decision. It closes D-054, open since ADR-0050 deliberately left frame pacing to
whichever card produced numbers.

**The measurements.** Linux/Wayland (KWin), RTX 4070 Laptop, 165 Hz, 960x540,
same scene, back to back:

| | fifo | mailbox |
|---|---|---|
| rate | 165 /s, pinned to the display | 1300–2500 /s |
| `frame` avg | 6.06 ms | 0.40–0.75 ms |
| `frame` worst | 7.4–8.1 ms | 4.7–5.5 ms settled |
| `update` avg | 0.01 ms | 0.00 ms |
| `gpu` avg | 0.02 ms | 0.02 ms |

**The scene cannot decide it on merit.** GPU time is 0.02 ms of a 6.06 ms frame,
so the program is idle for 99.5% of every FIFO frame and the display is the only
limit that exists. MAILBOX buys a frame up to one refresh interval fresher and
pays about fifteen rendered frames per visible frame for it.

**Two hard constraints.**

- **FIFO is the only present mode Vulkan guarantees.** MAILBOX is optional per
  surface and driver. A MAILBOX default is therefore not a default but a
  preference that silently resolves differently per machine.
- **ADR-0066's two halves pull in opposite directions here**, which is why this
  needed deciding rather than defaulting. *Nothing costly unless asked* argues for
  FIFO — MAILBOX is fifteen times the frames. *Never hide what it costs* argues
  against FIFO, which pins every reading to the refresh interval and makes the
  engine's real speed invisible.

## Options considered

### Option A — FIFO everywhere by default; uncapped is a request any program makes
The engine opens on FIFO. The dev program and the editor ask for MAILBOX at
startup, exactly as a game would. `render` learns nothing about build kinds.

### Option B — MAILBOX where offered, FIFO as fallback
The literal reading of *full blast*: the engine imposes no ceiling anywhere, and a
developer who wants to be capped turns vsync on.

### Option C — FIFO, and revisit when a scene costs something
The card's own recommendation: change nothing until the measurement means
something.

## Decision

**Option A, the principal deciding.** The deciding factor: uncapped frame rate is
not performance, it is consumption — it makes no frame cheaper, only more of them
— so ADR-0066's cost rule applies to it in full, while ADR-0066's visibility rule
is satisfied by the *programs a developer actually watches* running uncapped.

**1. `render` opens on FIFO.** It is the mode the specification guarantees, so
engine behaviour is identical on every machine, and it is the cheap one.

**2. Uncapped is not a second default; it is a request.** The dev program and the
editor call `voe_render_present_set(gpu, VOE_RENDER_PRESENT_MAILBOX)` at startup
and take the answer they get. A game does the same if it wants it. **`render`
carries no knowledge of which kind of build is running**, which is what keeps this
out of ADR-0052's three-builds distinction and out of the GPU layer entirely.

**3. `_get` reports what is in force, never what was asked for.** Already true in
the card's implementation, and load-bearing here: on a surface with no MAILBOX the
dev program runs on FIFO and its own timing block says so on every line. A
developer is never told a mode they are not in.

**4. This is the first knob of ADR-0064**, as that ADR predicted, and it is
rebuild-class: a change waits for frames in flight and may drop one. Under
ADR-0065 it drains at the frame boundary.

**5. Marked as a fast decision.** One call, one keypress, one line in a program's
startup. The evidence that would overturn it is a scene where GPU time is a real
fraction of a frame — at which point the table above is one press of `P` from
being re-measured, and re-measuring is the point.

## Blast radius

**Cheap.** Nothing structural depends on it; both modes are implemented and
tested, and the setting mechanism outlives either answer. What would be expensive
is the thing this decision avoids: teaching `render`, or the build system, which
kind of program is running, so that a "default" could differ per build. Rule 2
keeps that out.

Reversibility: **cheap.**

## Consequences

- **A developer's first number is honest.** The dev program shows real frame cost
  from the first run, which is ADR-0066 rule 3 delivered rather than asserted.
- **The dev program's startup changes**, and that is a card edit and therefore the
  principal's act: it starts on FIFO today and this decision says it should ask
  for MAILBOX. Small, and card 020 is in `review/` where it can be asked for.
- **The dev program will run hot**, deliberately: thousands of frames a second, a
  spinning fan, on a scene that costs nothing. That is the price of seeing the
  truth and it is paid by developers, not players.
- **Two machines will disagree about what the dev program did**, because MAILBOX
  is optional. Rule 3 makes the disagreement visible rather than silent, which is
  the best available outcome, not a fix.
- **A shipped game's default is FIFO, and most will never change it** — the right
  outcome for laptops and handhelds, and a thing to remember before reading any
  benchmark of a game built on this engine.
- **`IMMEDIATE` is not implemented and is not decided here.** True vsync-off with
  tearing is a third mode; it is neither offered nor refused, and the first card
  wanting it brings its own argument.

## Rejected options and why

- **Option B, MAILBOX everywhere** — rejected. Fifteen rendered frames per visible
  frame is unrequested spending, which ADR-0066 rule 1 forbids in the same breath
  it forbids a default post-processing stack; the fact that the spending buys
  frames rather than effects does not change what it is. It also makes the engine's
  default behaviour a function of the driver, since MAILBOX is optional.
- **Option C, decide nothing yet** — rejected, though it was the card's own
  recommendation and remains the right instinct about *the measurement*. What it
  misses is that FIFO-everywhere leaves ADR-0066's visibility rule unsatisfied in
  the one place it matters most: the picture the person building the engine looks
  at all day. The re-measurement Option C is waiting for still happens, and this
  decision does not close it — rule 5 names it.

## Questions this opens

- **D-088** — Whether `IMMEDIATE` is offered at all, and what it would be for given
  MAILBOX already removes the display's ceiling without tearing. Trigger: a card or
  a developer asking for it.

**Closes D-054**, open since ADR-0050.
