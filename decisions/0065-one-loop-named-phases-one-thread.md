# 0065. One loop with named phases on one thread; a differing rate is an accumulator, never a second loop

- **Status:** Accepted
- **Date:** 2026-09-05
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

The principal asked whether the engine should later have a draw loop, an update
loop and a physics loop, once it is more than a renderer.

The question arrived the same day the frame loop stopped being hypothetical.
Card 020 landed in `review/` with a real clock — `voe_platform_clock_now()`,
monotonic seconds — and a loop that already separates two named phases and
measures them apart: `update` (poll, input, three scene systems) and `draw`
(inside `voe_3d_draw_system_run`). Both run once per frame, on one thread,
stepped by however long the last frame actually took, with the scene's step
clamped at `MAX_FRAME_SECONDS` so a long hitch advances the world by a quarter
second rather than teleporting it. The unclamped number is what gets reported,
because what is reported is what happened.

**"Loop" is three separable decisions wearing one word**, and they must not be
answered together by accident:

| | Question | Status before this ADR |
|---|---|---|
| **Phase** | What runs, in what order, within one frame | Exists, unwritten |
| **Rate** | Whether something ticks at a cadence of its own | Unasked — everything is once per frame |
| **Thread** | Whether anything runs concurrently | Unasked — everything is one thread |

**Hard constraints already fixed.**

- **A system depends on data, never on another system** (ADR-0017). Intent is
  submitted as a value into a queue and the owning system drains it. This is the
  constraint that makes the whole question tractable: a differing rate is a
  question of *when a queue is drained*, not of who calls whom.
- **A component has exactly one writer** (ADR-0011). Physics, animation and
  gameplay all wanting to move something is the classic write conflict, and it is
  already solved by construction — they submit transform intent and the transform
  system applies it.
- **Rebuild-class settings need a drain point provably between frames**
  (ADR-0064), not merely "the next time that system runs". The loop owes this
  point a name; ADR-0064 deliberately left it to the loop.
- **`app` owns the frame loop and the wiring** (ADR-0030's map). `app` does not
  exist on disk yet; the loop currently lives in `dev/src/main.c`.
- **Implement on demand** (ADR-0034) and no premature generality (ADR-0008).
  There is no physics, and v1 has none.
- `ecs/include/ecs/intent.h` promises only *"the next time its system runs"*,
  which may be the same frame or the next depending on order — the weak form,
  pending this decision (D-031).

## Options considered

### Option A — one loop, named phases, one thread; a differing rate is an accumulator inside it

The frame is a fixed, named sequence of phases. Everything runs once per frame
except code that explicitly banks elapsed time and steps a fixed amount zero or
more times — the accumulator shape. Physics, when it exists, is the first and
probably only such phase; the draw interpolates between its last two states.
One thread throughout.

Costs: a heavy simulation directly costs frame rate, with no overlap to hide it.
Interpolation needs the previous state kept, which grows the transform component.
Makes easy: determinism, reasoning about order, debugging — one stack, one order,
every frame the same. Makes permanent: nothing that a later threading ADR could
not revisit, because the intent seam is where concurrency would enter and this
ADR does not close it.

### Option B — decide the phase order now, leave the physics rate to the physics card

Identical today. Settles only what ADR-0064 already forces — the order and the
between-frames point — and says nothing about fixed steps.

Costs: the physics card decides variable-versus-fixed on its own, under its own
deadline, and interpolation is not a thing retrofitted cheaply once a card has
shipped a variable-step solver that "works fine on this machine".

### Option C — a separate render thread, planned for now

Simulation and rendering on two threads, with the world double-buffered or a
command buffer between them. The shape most commercial engines end up at.

Costs: it changes ownership, drain rules and every system's assumptions at once;
it needs a snapshot discipline nothing here has; and the win is unmeasurable
until there is something heavy enough to overlap.

## Decision

**Option A, the principal deciding.** The deciding factor: the intent queue
already decouples *submitting* a change from *applying* it, so a differing rate
costs an accumulator and nothing structural — while a second thread costs the
ownership model, and buys a win nobody can measure yet.

**1. The frame is one loop, on one thread, with these phases in this order.**

```
one frame:
    sample      poll the window, read input. No world writes.
    submit      game, dev or editor code submits intent values.
  [ physics ]   later: fixed step, run zero or more times (see 3).
    step        the owning systems run in a defined order, each draining
                its own queue and applying what it drained.
    draw        the draw system reads the world and records the frame.
    present     the frame goes to the screen.
  ------------- the frame boundary -------------
```

**2. This answers D-031, in the strong form the weak one deferred.** An intent
submitted before `step` is applied in *that frame's* `step`, not the next. An
intent submitted *by* a system, to a system that has already run this frame, is
applied next frame. The order of systems within `step` is therefore load-bearing
and is the loop's to declare, not each system's to assume. `ecs/intent.h`'s
"next time its system runs" stays true and stops being the whole story.

**3. A phase wanting a different rate uses an accumulator inside this loop.**
It banks elapsed time and steps a fixed amount zero or more times per frame. It
does not get a loop, a thread, or a clock of its own. Two numbers are required of
any such phase and are that phase's ADR to set: **the fixed step size**, and **a
maximum number of steps per frame** — without the second, a machine that cannot
keep up runs more steps because it is behind and falls further behind for having
run them.

**4. When a fixed-rate phase exists, the draw interpolates.** Drawing the raw
fixed-rate state at a variable frame rate is visible judder, and it is the reason
people wrongly conclude fixed steps look worse. Interpolation needs the previous
state as well as the current one, and by ADR-0011 that second copy belongs to the
module that owns the component — the transform module, not the physics phase.

**5. Rebuild-class settings drain at the frame boundary**, after `present` and
before the next `sample`. That is the only point in this loop where "between
frames" is provable rather than asserted, and it is the point ADR-0064 named but
left for the loop to place. Free-class settings drain in `step` like any other
intent.

**6. One thread until an ADR says otherwise.** Not a claim that threading is
wrong. A card does not introduce a thread; a decision does. The door is
deliberately left open at the seam the ECS already built for it — intent queues
are safe to submit into from several threads and drain in one place — so nothing
here is paid for now to buy it later.

## Blast radius

**Moderate, and asymmetric.** Adding a phase is cheap and expected: `text`,
`sprite` and `ui` each add one, and each names its position. Adding the physics
accumulator later is a local change, precisely because intent already separates
submission from application — that is this decision cashing in ADR-0017.

What is expensive to reverse is **6**. Going multi-threaded later means deciding
how the render thread sees a consistent world, which is a snapshot or
double-buffering discipline that touches every component. This ADR does not make
that harder than it already is; it declines to make it easier.

Reversibility: **moderate** for the phase order, **load-bearing** for the single
thread — which is why the single thread is stated as a decision rather than left
as a fact about the current code.

## Consequences

- **The frame order becomes a written thing, and cards must place themselves in
  it.** A card that adds work says which phase it runs in. That is a new sentence
  three cards on the board will need.
- **A slow frame gets slower once physics exists.** More elapsed time means more
  fixed steps, and running them takes time. This is inherent to the shape; rule 3's
  maximum-steps number is the guard, and the honest failure is a simulation that
  falls behind in slow motion rather than one that freezes.
- **Interpolation costs memory and a copy per interpolated entity per frame.** The
  transform component grows. Named here so the physics card does not discover it.
- **One thread means a heavy simulation costs frame rate directly.** No overlap
  hides it. Accepted, and the measurement that would change our minds now exists —
  card 020's `update` and `draw` numbers are exactly the evidence a future
  threading ADR would argue from.
- **The loop lives in `dev/src/main.c` today, and `app` does not exist.** This ADR
  describes a shape that currently has one implementation in a program that is not
  the one the map says owns it. A second program — the editor, a game — would
  reimplement the order and could reimplement it differently. That drift is real
  and is the strongest argument for `app` arriving sooner rather than later.
- **The clamp and the accumulator are the same idea at different maturities.**
  `MAX_FRAME_SECONDS` exists because a variable step needs a ceiling; rule 3's
  maximum step count is that ceiling for the fixed-rate case. Whoever writes the
  physics phase should recognise it rather than invent a second mechanism.

## Rejected options and why

- **Option B, phases now and the rate later** — rejected, though it was close and
  costs nothing today. The reason to take the rate question now is that its cost
  falls on a *different* module than the one that would decide it: interpolation
  grows the transform component, and a physics card discovering that mid-flight
  either does it wrong or stops to ask. Deciding the shape now makes the physics
  card an implementation.
- **Option C, a planned render thread** — rejected. It is the answer to a
  measurement nobody has taken. It would rewrite the ownership model to buy
  overlap between a simulation that does not exist and a draw whose cost is
  already reported per frame. Revisit when the numbers show one phase idle while
  the other is the limit, and revisit it as its own ADR.
- **A thread per phase, as a general mechanism** — not seriously considered, and
  named here so it is not reinvented. It is ADR-0008's premature generality with a
  scheduler attached.

## Questions this opens

- **D-081** — Where the frame loop lives once more than one program needs it, and
  what `app` exposes: a function per phase, a callback, or a loop the program
  writes from parts. Relates to D-038, whose arenas `app` creates and hands down.
- **D-082** — The fixed step size and the maximum steps per frame, as numbers.
  Trigger: the first physics card.
- **D-083** — How the previous state for interpolation is represented, who writes
  it, and whether every transform carries it or only entities a fixed-rate phase
  touches. Owned by the transform module (ADR-0011), not by physics.
- **D-084** — Whether `text`, `sprite` and `ui` each add a phase or share one
  "renderers" phase, and where a phase that both reads the world and draws sits
  relative to `draw`. Trigger: card 021.

**Closes D-031**, open since ADR-0017 drew the consequence and deferred the
ordering.
