# 0485 — Work may run in controlled bursts on several threads
date: 2026-10-28
by: tech-lead

## Decision
The one-thread rule is dropped. Work that is worth splitting may run on several threads at once, in a
controlled burst. Import, cooking, loading, LODs, decoding and growing foliage are examples.
1. **A burst is bounded.** It starts for one piece of work, runs on a known number of workers, and ends when that
   work is done. No thread spins or polls between bursts, so an idle editor or game still costs nothing.
2. **One engine facility runs every burst.** Features do not each make their own threads. The planner designs it.
   The core count is read once, and a burst never takes every core.
3. **Frames stay steady (0386 holds).** Under a splash a burst may use all cores but one. While the program runs,
   a burst leaves the main thread and the GPU submission their time, and is paced so no frame hitches. A visible
   hitch is still a bug.
4. **Which work fans out is decided per feature.** It is decided where there is waiting worth removing, measured
   as 0358 asks, and never as a default for everything.
5. **098 may use it** for whatever part of the first open is not texture compression (LODs, decoding, baking).

## Reasoning
The sponsor (2026-10-28): "controlled bursts of multithreading is good when needed." 0456 kept loading on one
thread to avoid a pool's work, and 0483 rules that out as a reason. The first open of a small project took 15
minutes, partly because every model's steps ran in turn on one worker. Alternatives:
- Keep one thread and only make the work smaller. This stays slower as the hill grows.
- A permanent job system that everything runs on. This is wider than needed, and it risks hitches 0386 forbids.

## Replaces
Supersedes 0456. Amends 0454 point 3 and 0477 point 4 (cooking and compression on one thread), 0386's last
sentence (more threads are now allowed within this), and 0362/0370 (the splash worker may start bursts). The
"Loading models on every core" idea becomes this decision.
