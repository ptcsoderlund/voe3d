# 0448 — Models load in timed phases over a pool in base, held by the store
date: 2026-10-21
by: planner

## Decision
How 0447 is carried out, for 090 bug 02.

1. **The pool is `base/workers.h`.** C11 `thrd_t`, `mtx_t`, `cnd_t` (0370 point 1's reasoning). One
   call runs tasks 0 to n−1 and returns when all are done; the caller runs tasks too, so a pool
   of no threads, or NULL, runs them in order on the caller. A run from inside a task, or from
   two threads at once, is allowed: a waiting thread runs queued tasks of any run. A task gets
   its index, never a thread's number, so it makes its own arena: an arena per thread is unsafe
   once a waiter runs another run's task. A task runs under the caller's report subjects, and the
   kept error of the lowest-numbered task that kept one becomes the caller's when it has none.
2. **The size.** `platform/cores.h` answers the machine's logical cores; a program makes its pool
   with one thread fewer, the caller being the last. Made by the editor beside its model store
   and by the game's run; handed to the store (`voe_3d_models_workers_set`), which every game
   loader call reads it from, so no loader signature grows a pool but the levels make's.
3. **The phases.** The game's loader takes its files in chunks of the pool's threads, the
   caller counted, and runs each chunk through reading (stamp, bytes, `.parts`), parsing (the
   `.glb` with its pictures left encoded), making LODs (the levels file read, or made over the
   pool by level and part, and written), decoding pictures (one task a picture of the chunk),
   baking (3d's prepare: parts, own levels, made levels' indices, edges, sphere) and uploading
   (3d's put, one file at a time on the calling thread behind render's guard). Every phase but
   uploading touches no store and no device. Each file and each picture has an arena of its
   own, given back after the file's upload. A `.landscape` or picture path is read with the
   rest and loaded whole in uploading. Materials go reading, decoding pictures, uploading.
4. **The line.** A loader call that read a file writes one line to stderr, as the start log
   writes its: `game: models: <n> files in <s> s: reading <s> s, parsing <s> s, making LODs
   <s> s, decoding pictures <s> s, baking <s> s, uploading <s> s`, each the wall time of its
   phase over all chunks, to the millisecond; materials `game: materials: <n> files in <s> s:
   reading …, decoding pictures …, uploading …`. Not a report: nothing went wrong.
5. **Beside the frames.** In the editor, a model load between frames (new rows, the watch) goes
   through `game/models_loader.h`: the paths gathered between frames, reading to baking on a
   thread of the loader's over the pool while frames go on, then one file put between two
   frames each frame. A ready file the store now holds at its stamp, or a batch for another
   folder, is dropped. Splash waits keep the whole call on their worker. A material's, a
   landscape's re-read, the dot and the water stay between frames, short as they are; a file's
   own upload is what is left between two frames, its time the line's uploading.
6. **The weld.** The CPU's edges of a model weld positions through a hash of cells the weld's
   size, comparing a vertex with the representatives of its cell and the 26 around it, the
   lowest-numbered within the weld winning: the same answer as the quadratic walk, linear.

## Reasoning
Sampling the lumberhill start (release, 9.7 s of scene and shapes) put about 45 % in PNG
inflate and unfilter, 35 % in the quadratic weld of the CPU edges, 8 % in JPEG and 4 % in
uploads, all on one thread; import and Make LODs decode every picture again only to read the
geometry. Phases, not one task a file, keep a single big model spread over every core and give
each step one wall time to print. Alternatives: one task per file (a lone 108 MB model stays on
one core); a pool argument on every loader call (a signature change in 60 test calls); a job
system with priorities (more than loading needs, as 0362 said); uploads beside frames on the
loader's thread (the store is read by the main thread between frames, so it would need a lock).

## Replaces
Nothing. Carries out 0447; amends 0370 point 1 for loading as 0447 allows.
