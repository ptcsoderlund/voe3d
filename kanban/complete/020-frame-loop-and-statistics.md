# 020 — frame loop, honest timing, statistics to the console

status: review
claimed-by: claude-code (kanban-coder)
blocked-by: -

**Independent of every other card and can be pulled forward at any point.** It is
placed here because its value grows with how much there is to measure.

## Goal

A frame loop with real timing, printing honest numbers to the console.

## Why the console and not the screen

**There is no cheap way to put text on screen in this engine, deliberately.**
Everything is in 3D space (ADR-0049), so on-screen text goes through the full
texture-and-quad path like anything else. The principal's own note: more tools once
there is a GUI. Console now, on-screen when text properly exists.

## Scope

- **A real frame loop** with a measured delta time, not a fixed assumption.
- **Honest numbers.** CPU frame time and GPU frame time are different things and a
  single "fps" hides which one is the limit. GPU timing needs timestamp queries.
- **A rolling average and a worst case.** An average alone hides stutter, and stutter
  is what a person actually notices.
- **State clearly what is being measured**, in the output. A number whose meaning is
  ambiguous is worse than no number.

## This card decides frame pacing

The register carries it as open, waiting for exactly this card. **FIFO versus
`MAILBOX`** — the other thing "triple buffering" means: not waiting for the
display, discarding stale frames for lower latency.

- ADR-0050 deliberately left it open because it needs measurements to argue from,
  and this is the card where those exist.
- `MAILBOX` is **optional** — not every driver offers it, so a fallback to FIFO is
  required, not nice to have.
- Present the numbers with a recommendation. **This is a decision for the principal,
  not for the card** — report, do not choose.

## Verify

- Numbers are stable and plausible while nothing changes, and move when something
  does.
- Numbers are the same order of magnitude as an external measure — the compositor's
  own, or a stopwatch on a deliberately slow frame.
- `check.cmake` zero. Windows is the principal's, and the numbers will differ there,
  which is itself worth reporting.

## Notes (coder, 2026-09-05)

Done. Everything the card asks for is in, and the frame-pacing report the card
commissions is the last section below — **read that one, it is the card's actual
output and it wants a decision.**

### What was made

- **`platform` — a clock.** `voe_platform_clock_now()`, monotonic seconds as a
  `double`, `clock_gettime(CLOCK_MONOTONIC)` on Linux and
  `QueryPerformanceCounter` on Windows. `platform/tests/clock.c` is the claim:
  it moves, and it never goes backwards. Nothing in that test measures a
  duration against a duration, because a test that slept ten milliseconds and
  asserted ten is a test that fails on a loaded machine for reasons that are not
  the code's.
- **`platform` — `VOE_PLATFORM_KEY_P`.** One line in the enum and one in each
  backend's table, which is what `include/platform/input.h` already said adding
  a key costs. It is the present-mode toggle and the only non-movement key.
- **`base` — the `samples` module.** `voe_base_samples` is a count, a sum and a
  worst; `_reset`, `_add`, `_average`. Compact pure data with support functions
  and no system, the standing a maths type has. It is a **period and not a
  sliding window** — reset, add, read, reset — because that is what a reporting
  loop wants and it needs no ring buffer, no capacity to pick and no `count`
  that stops meaning what it says. `base/tests/samples.c` covers the one
  mistake here that looks right: a `worst` that survives a reset, which would
  report one stutter for ever.
- **`render` — GPU timestamps.** A two-query pool per frame slot, written at
  TOP_OF_PIPE and ALL_COMMANDS inside the frame's command buffer, read at the
  top of the next frame that lands on that slot — the first moment the fence
  says the card has finished writing them. `voe_render_frame_gpu_time()` is the
  public read. Readings are masked to the queue's `timestampValidBits` before
  they are subtracted and a wrap is treated as a wrap; a queue may report as few
  as 36 bits and the bits above are rubbish, not zeroes.
- **`render` — a present mode.** `voe_render_present`,
  `voe_render_present_set/_get`. **A device opens wanting MAILBOX** — see the
  decision recorded at the bottom of this card — and the swapchain honours it
  where the surface offers one, falling back to FIFO where it does not. `_get`
  reports what is in force and not what was asked for, so a surface with no
  mailbox says `fifo` and that is an answer rather than a failure. Five loader
  entries added.
- **`dev` — the frame loop.** `NOMINAL_FRAME_SECONDS` is gone. Every frame is
  stepped by what the clock says the last one took, and four numbers are printed
  every two seconds, each an average and a worst over exactly that period:
  `frame` (top of loop to top of loop), `update` (poll, input, the three
  systems), `draw` (inside the draw system — on fifo the wait for the display is
  in here), `gpu` (the card's own clock). A legend is printed once at startup
  because a number whose meaning is ambiguous is worse than no number. P toggles
  the present mode and every block says which is in force.

### One decision taken, and it is marked in the code

`MAX_FRAME_SECONDS` clamps **what the scene is stepped by** to a quarter of a
second. Nothing clamps what is reported — the samples get the real interval,
always. Without it a two-second frame (dragged to another monitor, a breakpoint,
the machine swapping) teleports the orbit a sixth of the way round in one step,
and what a person sees is a scene that jumped rather than a frame that was slow.
It is a real frame loop's clamp and not scope the card did not ask for, but it is
a judgement and it is written down at the constant.

### Verified — Linux / Wayland (KWin), NVIDIA RTX 4070 Laptop, 165 Hz

- `cmake -P check.cmake` **exits zero**, every step: 27 tests (25 before, plus
  `base/samples` and `platform/clock`), the analyser clean over 76 files, all
  ten folders configuring standalone.
- **The rate matches an external measure.** fifo reports 164.9–165.0 frames per
  second and a `frame` average of 6.06 ms against a 165 Hz display's 6.06 ms
  interval. That is the card's "same order of magnitude as an external measure",
  and it is closer than that.
- **The numbers are stable while nothing changes** — three consecutive periods
  at 330 frames, 6.06/6.06/6.07 ms — **and they move when something does.** The
  window built at 2560x1440 instead of 960x540 took `gpu` from 0.02 ms to
  0.06 ms, which is the one number that should follow the pixel count and does.
  (That was a temporary edit to measure with; it is not in the diff.)
- **Both present modes measured**, the second by temporarily starting in
  MAILBOX rather than by pressing P, which a script cannot do. Numbers below.
- **Not checked here**, and each is a note for whoever runs this next rather
  than a gap: **Windows** — the whole of `clock_win32.c` and the `P` row in that
  backend's key table compile nowhere on this machine and have never run; the
  numbers there will differ and that is itself worth reporting. **Pressing P**
  — the toggle path through `voe_platform_input_key_down` was not exercised by a
  finger, only the `voe_render_present_set` call underneath it. **A card that
  cannot write timestamps** — the "no measurement" line has no hardware here to
  print it.

### The frame-pacing report — FIFO or MAILBOX

Measured back to back on the machine above, 960x540, the same scene:

| | fifo | mailbox |
|---|---|---|
| rate | 165 /s, pinned to the display | 1300–2500 /s, uncapped |
| `frame` avg | 6.06 ms | 0.40–0.75 ms |
| `frame` worst | 7.4–8.1 ms | 4.7–5.5 ms once settled |
| `update` avg | 0.01 ms | 0.00 ms |
| `gpu` avg | 0.02 ms | 0.02 ms |

**What the numbers say, and the honest reading is that this scene cannot decide
it.** `gpu` is 0.02 ms and `update` is 0.01 ms, so the program is idle for
99.5% of every fifo frame: on this hardware, with this scene, the display is the
only limit there is and nothing else is close. MAILBOX therefore buys latency —
a frame reaching the screen fresher, by up to one refresh interval — and pays
for it by drawing about fifteen frames for every one anybody sees, which is real
power and heat on a laptop for a picture nobody can tell apart.

**The coder's recommendation was to stay on FIFO** — the gain is invisible on
this scene and the cost in power and heat is not, and nothing in the engine yet
has input where a refresh interval is felt. **The principal decided otherwise and
the decision is below.**

**Two things the principal should know before deciding.** MAILBOX's first two
periods here showed a 30–35 ms `worst`, which is the swapchain rebuild settling
and not steady-state stutter — the third period onwards is clean. And a fifo
`worst` of 7.4–8.1 ms against a 6.06 ms average is about one refresh of
occasional overshoot, which is what a compositor doing other work looks like and
is not the engine.

### Decided by the principal, 2026-09-05: MAILBOX by default

**Uncapped by default; the engine goes MAILBOX.** The reason given is the
engine's standing philosophy and not this table: **performance by default.**
Nothing waits for anything it does not have to, and whatever costs
time — post-processing, real-time global illumination, whatever comes
next — is added deliberately by the developer who wants it, rather than being
paid for by everyone who does not. A default that waits for the display is a cost
imposed on every scene in advance, which is the shape of decision this engine
does not make.

Implemented in the same change, so the diff and the record agree:

- `voe_render_device_new` opens with `present_wanted = VOE_RENDER_PRESENT_MAILBOX`.
  It is what is *wanted*: a surface with no mailbox still gets FIFO, `_get` says
  so, and nothing fails.
- `VOE_RENDER_PRESENT_FIFO` stays `0`, so a zeroed `voe_render_present` is still
  the mode every driver has to support.
- `dev` starts with `mailbox_wanted = true`, so P asks for FIFO rather than for
  what is already happening.
- The reasoning is written at the enum in `render/include/render/device.h`, at
  the assignment in `device.c` and at the top of `swapchain.c` — three sites,
  because a reader arriving at any of them should not have to find the other two.

Measured after the change: `present mailbox` at startup, and 1800–2000 frames per
second against FIFO's 165. **This decision is architecture and its ADR is the
tech lead's and the principal's, at the planning root — a coder may not write
one, and this card is not a record of a decision, only of the code that carries
it out.**

### Markers

None. No `DEVIATION:` and no `BLOCKED:` — nothing in the card needed a folder
edit that could not be made, and the one judgement taken (`MAX_FRAME_SECONDS`)
is described above and commented at the constant rather than left silent.

One thing tidied that was not this card's: `kanban/todo/math_init.md` was an
empty file staged as a new add and absent from the working tree, left behind by a
pull. It was never committed, so unstaging it removed it entirely. The principal
confirmed it does not belong in `todo/`.

### Suggestion, not in the diff

`draw` is one bracket around the whole of `voe_3d_draw_system_run`, so on fifo it
reads as "6 ms" when almost all of that is the fence and the acquire blocking.
Splitting it needs `3d` or `render` to expose where the wait ends, which is a
card in one of those folders and not this one's to widen into.
