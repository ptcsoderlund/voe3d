# 0487 — A burst is platform's workers, started for one piece of work and joined after it
date: 2026-10-28
by: planner

## Decision
0485's one facility, and what feature 098 fans out on it:

1. **`platform/burst.h`**: `voe_platform_cores()` (logical processors, read once, at least 1),
   `voe_platform_burst_workers(splash)` (under a splash cores − 1, else cores / 4; at least 1),
   and a burst: `_start(workers, count, task, context)`, `_done` (no wait) and `_finish` (waits,
   joins, frees). A task is a function given the context and an index; workers take indices in
   order from one atomic counter. C11 threads, one source for both systems; only the core count
   is per system.
2. **No thread outlives its burst.** Workers are made at start and joined at finish, so nothing
   waits or spins between bursts. The caller runs no task, so a running program's main thread
   keeps its core; only when no worker will start do the tasks run on the caller, inside start.
   A start never fails.
3. **In the model batch** (`game/src/models_batch.c`) reading, parsing, making LODs, decoding
   (each picture decoded, and mipped by use when it will be cooked) and baking run as a burst
   over their tasks; compressing, the copy's write and the put stay on the batch's thread, which
   owns the device. A batch given a progress is under a splash and uses the splash count, and the
   loaders hold up to `min(4, workers)` files there instead of one. Folder making treats a folder
   that already exists as made.
4. **In a material's load** its maps are read, decoded and mipped as one burst, a task a map.
5. **The editor's queue** (0489) runs its read, decode, mip and write on a one-worker burst.

## Reasoning
090 bug 13 measured decoding at 5.1 of 5.7 s for one model in release; once compression is on the
GPU it is the wait. Pictures decode independently, so a task a picture; up to four files at
once lets a model with one picture still fan out, and four bounds the memory of 4096 maps.
A pool of parked threads would be 0485's rejected permanent system. A quarter of the cores while
running leaves the main thread and the GPU submission theirs (0386).

## Replaces
Nothing; designs 0485 points 2 and 4.
