# 0119. One loader thread, and the frame loop stays one thread

- **Status:** Accepted
- **Date:** 2026-09-11
- **Deciders:** Human, Tech Lead
- **Supersedes:** — (exercises ADR-0065 rule 6's *until an ADR says otherwise*; every
  other rule of ADR-0065, including that the frame is one loop on one thread with its
  phases in order, stands unchanged)
- **Superseded by:** —

## Context

ADR-0118 settled what asynchronous loading means in this engine. This decides how the work
leaves the frame's critical path, and it is a separate ADR because the contract is
load-bearing and this is not: callers see a request and an id whichever mechanism is
underneath.

ADR-0065 rule 6 requires this ADR to exist at all — *"One thread until an ADR says
otherwise. Not a claim that threading is wrong. A card does not introduce a thread; a
decision does."* This is that decision, and it is deliberately the smallest form of it.

The constraint that decides the option is in the decoders. `assets` holds PNG, JPEG,
DEFLATE and a glTF reader, all written flat and single-pass on purpose — `src/png.c`'s
header says why it has no recursion in it, and the JSON reader has an explicit stack for
the same reason. They are correct, tested against hand-built hostile inputs, and none of
them can be suspended halfway.

The second constraint is that ADR-0118 rule 6 already requires the GPU's wait-for-idle to
go. A wait-for-idle stalls the device rather than the calling thread, so **no option here
removes the stutter on its own** — that is a `render` change either way, and it is D-227.

## Options considered

### Option A — time-sliced inside the one loop

A loading phase in the existing frame that does at most a budgeted couple of milliseconds
per frame. Asynchronous from the caller's point of view — request now, ready later —
with no second thread, ADR-0065 rule 6 untouched, no data races, and ADR-0032's
thread-safety question never asked.

The cost is that time-slicing only works on chunkable work, and decoding one large image
is not chunkable. It means rewriting PNG, JPEG, DEFLATE and the glTF reader as resumable
state machines, and every decoder written afterwards pays the same tax. It also cannot
slice a single blocking read of a large file, which is half the latency.

### Option B — one loader thread

One named thread whose whole job is loading: read, decode, hand the result over. The
existing decoders run unchanged because each call gets its own arena, which is the shape
ADR-0032 rule 6 already prescribes — *"one arena per thread or per job"*. The completed
result reaches the frame through the intent queue, which ADR-0013 and ADR-0017 built for
precisely this and record as safe to submit into from several threads and drain in one
place.

The frame loop itself is untouched: same phases, same order, same one thread.

### Option C — a job system

A general worker pool with loading as its first client. More capable, and it is what
ADR-0032 rule 6 names as the trigger to revisit arena thread-safety properly.

Its honest cost is a scheduler nobody has specified, serving exactly one consumer.

## Decision

**Option B, one loader thread.** The deciding factor is the decoders: Option A's real
price is not the loader but rewriting four correct, tested, deliberately flat files into
coroutines, to buy a property a thread gets for nothing.

1. **One thread, with one job.** It reads bytes and decodes them. It does not touch
   Vulkan, does not touch the ECS, and does not run systems.
2. **The frame loop is unchanged.** ADR-0065's phases, their order and their single thread
   all stand. This ADR lifts rule 6 by exactly one thread and no further; a card still may
   not introduce another.
3. **Each load gets its own arena** (ADR-0032 rule 6). No arena is shared across the seam
   and no locking is added to `base`.
4. **Results cross into the frame through the intent queue** and are drained in one place,
   as ADR-0017 specifies. Nothing on the loader thread writes engine state directly.
5. **This is not a job system.** When a second thing wants a thread, that is ADR-0032 rule
   6's revisit condition and a new decision, not an extension of this one.

## Blast radius

**Reversibility: cheap**, and this is the reason it is a separate ADR from ADR-0118.
Because a caller sees a request and an id either way, replacing one loader thread with a
job pool later touches no call site — it is an internal change behind an API the contract
already fixed. Getting this wrong costs a rewrite of one file's worth of plumbing.

What is *not* cheap is rule 2: once more than one thread exists, the claim that the frame
is single-threaded stops being a fact about the program and becomes a rule somebody has to
keep. That is the part worth guarding.

## Consequences

- **The engine is no longer single-threaded, and every future reader of ADR-0065 needs to
  know rule 6 was exercised here.** That is the point of recording it rather than letting a
  card do it.
- **`base` gains no locking and no atomics**, which is only true as long as rule 3 holds.
  The first shared arena breaks ADR-0032 rule 6 silently, at runtime, on someone else's
  machine.
- **Data races become possible for the first time in this codebase**, and nothing in the
  toolchain currently looks for them. Whether the check script grows a sanitiser build is
  a real question and is opened below.
- **This alone removes no stutter.** ADR-0118 rule 6 and D-227 are where the GPU stall
  goes; a loader thread that ends in a wait-for-idle has moved the work and kept the spike.
- **Debugging gets harder in a way that is hard to price.** A load that completes between
  two frames makes a class of bug that reproduces on one machine and not another, and the
  one platform nobody can run an agent on is the principal's Windows box.
- **The thread exists even when nothing asynchronous is loaded.** Whether it is started
  lazily on the first asynchronous request or with the device is an implementation detail a
  card may settle; it is not worth an ADR.

## Rejected options and why

- **Option A, time-slicing.** Rejected because the decoders cannot be suspended and making
  them suspendable is a rewrite of four working files plus a permanent tax on every decoder
  after them. It is the option that keeps ADR-0065 rule 6 intact and that is genuinely
  worth something, but not this much. It is also the only option that cannot slice a
  single large blocking read.
- **Option C, a job system.** Rejected as premature: a worker pool with one caller, and a
  scheduler that would have to be specified before the first asset loads. ADR-0032 rule 6
  already names when to revisit this, and the answer then will be better informed than it
  would be now. Option B is cheap to replace with it precisely because the contract sits
  above both.

## Questions this opens

- **D-230** — whether `check.cmake` grows a thread-sanitiser or race-detector build now
  that races are possible, and on which platforms it can.
