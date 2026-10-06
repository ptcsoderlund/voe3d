# 0370 — A splash wait is one C11 thread, a guarded device and a cache in settings
date: 2026-10-06
by: planner

## Decision
For 064, carrying out 0362:
1. **Threads are standard C.** The worker is a `thrd_t` from `<threads.h>`; shared flags and counts are
   `<stdatomic.h>`; the device's guard is an `mtx_t` and a `cnd_t`. They are the C library's, not an OS
   header, so no `platform` wrapper. Only `render` (the guard) and `game` (the wait) name them.
2. **The wait.** `game/starting.h` runs a work function with a context on one worker while the main
   thread polls, draws splash frames with the progress line and nothing else; a window that is not
   visible draws no frame and waits on the window instead. A close sets the stop flag, joins the
   worker and answers false. The work function and its context are the thread's entry, the one way
   `<threads.h>` starts one.
3. **Progress** is `game/progress.h`: a phase, done and total, and a stop flag, all atomic. The line is
   "phase done/total", or the phase alone when total is 0. A worker checks stop before every step: a
   pipeline, a model file, a scene.
4. **The split.** Game start: the shaders step, then the world made and the project registered, the
   scene built, the shapes uploaded and the models read. Editor start: the shaders step, the shapes
   uploaded and the models read; the project is still opened before the window, where nothing can
   freeze. Editor New and Open: the session's load and the models read. The sound device, the start
   log's writing and everything after the hand-off stay on the main thread.
5. **The device's guard.** A frame is open from `voe_render_frame_begin` to `voe_render_frame_end`. A call
   that uploads, frees or writes a pool, table or descriptor set, or uses the queue, waits while a frame
   is open on another thread and holds the guard throughout; frame begin and end take it too. So an
   upload never overlaps a frame being recorded or submitted. Pipeline builds take no guard: they
   write only their own handles and the cache, which no element frame reads; the relight's startup,
   which allocates and writes, takes it.
6. **The cache.** `render` hands its pipeline cache out as bytes behind its own header (magic, the card's
   vendor, device and pipeline cache UUID, a hash of every embedded shader, the payload's size and a
   checksum) and takes such bytes back before the first prepare step, starting empty on any mismatch.
   `app/pipeline_cache.h` keeps them at `<settings>/voe3d/pipelines_editor.cache` and
   `pipelines_game.cache`, read before the shaders step and written atomically once it is PREPARED;
   a stopped or failed step writes nothing. Any read or write failure is at most a stderr line.

## Reasoning
`<threads.h>` is in glibc and the Windows CRT, so a wrapper would add a file and no meaning. Excluding
uploads from open frames, not just from each other, is what makes rewriting the texture array safe: a
set may not change while a recorded frame binds it, and a guard per queue call would allow that. The
settings folder exists on every machine the engine runs on and is never tracked or shipped; separate
editor and game files keep two different engine builds from rewriting one file in turn.
Alternatives: uploads on the main thread between frames (one large texture still freezes it); a
guard taken per queue call (unsafe for the texture array); the cache beside the program (a shipped
game's folder may be read-only, and it would ship).

## Replaces
Nothing. Carries out 0362; replaces 0345 point 5 (no pipeline cache of our own).
