# 0135. `app` is parts a program calls in its own loop

- **Status:** Accepted
- **Date:** 2026-09-11
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

**Closes D-081**, open since ADR-0065: *where the frame loop lives once more than one
program needs it, and what `app` exposes — a function per phase, a callback, or a loop the
program assembles from parts.* ADR-0121 made the editor a second program and `dev_editor` a
third, and put `app` on the path. The first editor card cannot be written until this is.

Fixed by earlier decisions. ADR-0065: one loop, one thread, phases in the order *sample,
submit, step, draw, present*, and **the order of systems within `step` is the loop's to
declare**. ADR-0098: the program opens and closes the frame, and D-081 was narrowed to
*parts, not a callback*. `voe3d/CLAUDE.md`: function pointers are expected only in
`render`'s Vulkan table. ADR-0131: a program asks for its present mode itself.
`cmake/voe.cmake` has a row for `app` — `base math ecs scene platform assets render 3d` —
and there is no folder on disk.

What `dev/src/main.c`'s loop shows, read the same day. **About forty lines are the same in
any program**: measure the frame's interval and clamp what the scene is advanced by, poll
the window, read its size, stop the clock while it is minimised, open the frame, handle a
begin with nothing to draw into, close it, and stop when the device stops answering.
Everything else is `dev`'s own. **And one placement in it is load-bearing:** the heads-up
line is submitted *between* the camera system and the transform system, because placed
earlier it is a frame behind the camera and reads as jitter. A program must be able to act
between two systems.

## Options considered

### Option A — no `app` yet
The editor writes its own loop from ADR-0065's order. Nearly free now; every program and
every game written on the engine re-derives the forty lines that are easiest to get subtly
wrong.

### Option B — `app` is parts
A few functions for exactly the shared lines. The program writes its own `while`, calls the
parts, and runs the systems itself in its own order.

### Option C — `app` owns the order
A step call runs the engine's systems in a declared order; the program submits before it
and draws after. Breaks `dev`'s between-systems placement, and a program's own systems need
a slot among the engine's — a callback, which is ruled out, or a step split into pieces,
which is Option B.

## Decision

**Option B**, the principal's choice on the tech lead's recommendation. The deciding
factor: **a program must act between two systems, which rules out C, and the lines every
program shares are the ones easiest to get wrong, which rules out A.**

1. **`app` holds parts and no loop.** The program writes the `while` and calls them. Nothing
   in `app` calls the program back.
2. **Three parts, named here by what they do; their spelling is the card's.**
   - **Startup**: open a window and a device from the program's size, title and capacities,
     and hand both back — failure returned, per rule 13.
   - **Frame open**: poll; measure the interval; report the raw interval, the clamped step,
     the window's size, whether it is minimised and whether closing was asked for.
   - **Draw open and draw close**: around `render`'s frame begin and end, including the begin
     that has nothing to draw into and a device that has stopped answering.
3. **Not in `app`: the order of systems, the program's world and registrations, arenas, the
   measurement readout, key bindings, and the present mode request.** Each stays with the
   program until a second program needs the same one.
4. **The existing `app` row is enough.** No dependency edge is added. The editor's row names
   `app`; no folder names the editor (ADR-0121).
5. **`dev` uses `app`.** It is the program that proves the engine without the editor
   (ADR-0121), so it is also the one that proves `app` suffices for a game with no editor.

## Blast radius

**Cheap.** Reverting to A puts forty lines back in each program. Growing toward C later is
an additional convenience on top of the parts and removes nothing. What would be expensive
is point 1's direction: an `app` that calls the program has taken over the loop, and every
program is then written around it.

## Consequences

- **The editor's loop is short and its own**, and the editor runs the owning systems every
  frame, as ADR-0134 point 7 requires.
- **Two cards before the editor card**: `app` is written, then `dev` switches to it. Split
  because a card owns one folder, and `dev` is the first caller rule 10 asks for.
- **The order of systems is still repeated in every program**, and can differ between them.
  Accepted: `dev` shows that a program legitimately interleaves its own work between them.
- **D-038 does not fire.** Its condition was *the `app` card*; the arenas stay with the
  program under point 3, and it waits for a program needing an arena lifetime it cannot
  make itself.
- **D-086**, where the always-available measurement lives once `app` exists, is answered
  *with the program* for now by point 3, and keeps its row.

## Rejected options and why

**A — no `app`.** ADR-0121 counted three programs, but the real count is every game built
on this engine, and each would re-derive the clamp, the minimised window and the empty
frame. The forty lines are cheap to write and expensive to write slightly differently.

**C — `app` owns the order.** It breaks a placement `dev` needs today, and its only escape
for a program's own systems is a callback or a split step. The first is ruled out and the
second is B.

## Questions this opens

None.
