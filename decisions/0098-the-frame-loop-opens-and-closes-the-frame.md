# 0098. The frame loop opens and closes the frame; the draw system draws into it

- **Status:** Accepted
- **Date:** 2026-09-08
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

**Found by the coder on card 028**, and reported as a contradiction rather than
patched, which is the right act. The card says `voe_render_geometry_create_transient`
requires an open frame and asserts without one, because the slot it writes into
is not known until `voe_render_frame_begin` has picked it (ADR-0084). The only
code that opens a frame is `3d`'s draw system: `voe_3d_draw_system_run` calls
`voe_render_frame_begin`, walks the tables and calls `voe_render_frame_end`, and
its header states the contract as *one frame, one call*. The dev loop calls it as
one function. **So there is no moment at which the dev program can build the
statistics text** — before the call the frame is not open, after it the frame is
already presented. Card 028's proof, the readout on the screen, cannot be built
as written.

**This is not a card 028 problem.** Three cards already on the board or in the
plan have the same shape:

- **030, the element buffer** (ADR-0092) is *a per-frame-slot host-visible
  buffer of element records* — the transient pool's shape again, and written
  every frame.
- **032, panels are entities** (ADR-0093) has the draw system walk a second
  table. The element buffer a panel names must be filled before that walk and
  after the frame has picked its slot.
- **033, the `ui` context** (ADR-0091) is immediate mode: widget calls each frame
  emit elements. Those calls are the program's — a game's, the editor's, the dev
  program's — and they have to run inside an open frame, before the walk.

So the question is: **who owns the open frame — the program's loop, or the draw
system?** ADR-0065 put `draw` and `present` as the last two phases of one loop
on one thread and left the inside of `draw` to the code. ADR-0084 and 0085 made
geometry that changes per frame a real thing. Those three together already
answer this; what is missing is saying so before three cards each discover it.

**Hard constraints.**

- ADR-0084: transient memory is per frame slot, and the slot is chosen by
  `voe_render_frame_begin`. Nothing may write transient memory before that, and
  a wait inside the create is the card's named bug sign.
- ADR-0085: a mesh's geometry is replaced by a direct call. That is the only
  world write that legitimately happens after `step`, and it exists for exactly
  this path.
- ADR-0065: one loop, phases in order, `draw` then `present`; rebuild-class
  settings drain at the frame boundary after `present`. Nothing here may add a
  phase, a thread, or a second loop.
- ADR-0093: `3d` never names `ui`. Whatever fills an element buffer, it is not
  the draw system.
- `render`'s own header example (`render/include/render/device.h`) already shows
  the loop calling `_begin`, drawing, and calling `_end`. The draw system's
  wrapping of that pair was a convenience taken when nothing else needed the
  frame open.

## Options considered

### Option A — the loop opens and closes the frame; the draw system draws into it
`voe_render_frame_begin` and `_end` move out of `voe_3d_draw_system_run` and
into the program's loop, which is what `render`'s header has shown all along.
The loop, inside ADR-0065's `draw` phase: begin the frame; build what changes
this frame — the statistics text through the transient pool and `_set_geometry`,
later the GUI's element buffer through `ui`; run the draw system into the open
frame; end the frame, which presents. The draw system's contract becomes *draws
the world into the frame that is open* and its `size` parameter goes with the
begin call. A begin that comes back with nothing to draw into is the loop's to
handle, exactly as the header already documents.

Costs: two lines in every program's loop, forever, and the no-area case handled
by the loop instead of inside `3d`. Makes easy: every future per-frame producer
— text, elements, a debug line renderer — has one obvious place to run and it is
the program's own code. Makes permanent: the frame is a state the *program*
owns; `3d` is one consumer of it, not its keeper.

### Option B — the create may run between frames and waits on the next slot itself
`_create_transient` learns which slot the *next* `_begin` will pick, waits on its
fence, writes, and `_begin` finds the memory already used. The draw system keeps
its one-call contract and the dev program builds the readout before calling it.

Costs: the create now contains a fence wait, which card 028 names as the sign
the pools are not per-slot after all. The wait is then done twice, once here and
once in `_begin`. The slot is chosen in two places, so a `_begin` that turns back
without spending its slot — no area, stale swapchain — has to know that
transient space was already consumed on a slot that never went in flight, or
leak it for a frame. It also does nothing for `ui`: widget calls that emit
elements between frames would be writing a slot's buffer while `_begin` has not
yet promised that slot is free, the same hazard moved one folder up.

### Option C — the draw system takes a callback it runs after `_begin`
`voe_3d_draw_system_run` gains a function pointer and a context, called once the
frame is open and before the walk. The one-call contract survives in letter.

Costs: it is the *callback* answer to D-081 taken by accident, on the first card
that needed anything inside the frame. The GUI's widget calls — the body of every
program's per-frame code under immediate mode — would live inside a callback
invoked by `3d`, which inverts who owns the frame and is the shape immediate-mode
GUIs exist to avoid. It also hides the frame from the loop that ADR-0065 says
owns the phase order.

## Decision

**Option A, the principal deciding 2026-09-08.** The deciding factor: **ADR-0065 made the loop the owner
of the phase order, and the frame being open is a phase state, not a draw-system
detail** — the draw system was holding the frame only because, until this card,
nothing else needed it open.

What this pins:

1. **`voe_render_frame_begin` and `voe_render_frame_end` are called by the
   program's frame loop**, not by `3d`. The draw system draws into the frame
   that is open and asserts if none is, the same rule `render` already applies
   to `voe_render_frame_draw`.
2. **The inside of ADR-0065's `draw` phase has an order**, and it is: open the
   frame; build what changes this frame; the draw system walks the world; close
   the frame, which is `present`. Building what changes is where ADR-0085's
   direct geometry call is made and where, later, `ui`'s per-frame calls run.
   It is the only world write after `step`, and it is permitted because
   ADR-0085 already permits it.
3. **A frame that opens with nothing to draw into is the loop's case**: no
   transient build, no draw system, no `_end`. `render`'s header already says
   so; the draw system stops saying it on `render`'s behalf.
4. **The draw system's `size` parameter is gone**; the size belongs to `_begin`,
   which is where it always was consumed.
5. **The dev program's timing samples** keep their names. `draw` now spans from
   `_begin` to `_end` inclusive of the transient build, and the readout says so.
   The comment at `dev/src/main.c:1351` describing what is inside the draw
   system is rewritten to describe what is inside the loop's `draw` phase.

## Blast radius

**Cheap in code**: two calls move one level up, one parameter is removed, one
header's first sentence is rewritten. Every program that exists is the dev
program.

**Load-bearing as a contract**: after this, every program built on the engine
writes the open/close pair itself, and every module that produces per-frame data
is designed against *the loop has the frame open*, not *the draw system will
open one for me*. Reversing it later means moving that responsibility back into
`3d` after `ui` has been built assuming it is not there.

Reversibility: **cheap** now, **load-bearing** after card 033.

## Consequences

- **Card 028 is unblocked** and amended: the dev scope builds the readout
  between `_begin` and the draw system call. The coder's four pending items —
  the readout, the capacity measurement, the rebind count and the cost figure —
  all follow from this.
- **Cards 030, 032 and 033 do not each rediscover this.** 032's second table is
  walked inside the same open frame; 033's `ui` begin/end sit between the
  render `_begin` and the draw system with nothing to invent.
- **The dev loop grows two lines and one branch**, and so will the editor's and
  every game's. That is the price of the loop owning its phases, and ADR-0065
  chose that price knowingly when it refused to hide the phases behind a single
  call.
- **`3d`'s draw system loses the ability to be *the* frame** — a program cannot
  call one function and have a picture. That convenience was real and it is
  gone; a helper that does the whole sequence is exactly the kind of thing `app`
  may one day offer, and D-081 is where that is decided.
- **The consequence I do not like:** `drawing == false` handling is now
  something every loop author must get right, where before one file did it. The
  header documents it and the dev program shows it; that is the mitigation and
  it is documentation, not enforcement.

## Rejected options and why

- **Option B, create between frames** — rejected because it puts a fence wait
  inside the create, which is the card's own definition of the bug, and because
  it splits slot choice across two calls. It also does not help the GUI at all.
- **Option C, a callback** — rejected because it decides D-081's shape by
  accident in the wrong direction, and because it puts the program's per-frame
  GUI code inside a call made by `3d`, inverting ownership of the frame.

## Questions this opens

- **D-081 narrowed, not closed**: whatever `app` exposes, the frame's open and
  close are functions the loop calls in sequence, not a callback `app` invokes.
  Where the loop lives is still open.
- **D-084 touched**: building what changes is a step inside `draw`, not a phase.
  Card 021b's expected answer — no new phase — holds here too.
- No new row.
