# 0362 — A splash wait runs on a worker, and prepared shaders are cached
date: 2026-10-05
by: tech-lead

## Decision
Whenever the splash is up (0346), in the editor or a game, at start or on a scene load, the main thread does
only three things: it handles the window's events, draws the splash, and updates its status line. The heavy work
runs on one worker thread: preparing shaders and pipelines, reading and decoding the scene and its models and
textures, and uploading them. The worker reports progress the status line can show, for example "Preparing
shaders 12/40". It hands its results to the main thread once the work is done, and the main thread is the only
one that touches the world afterwards. Outside a splash wait, nothing new runs off the main thread. This is no
general job system. Closing the window during a splash wait quits within about a second. The worker stops at
its next step, never halfway through a GPU call, and nothing half-written is left behind. Prepared shaders are
kept in a pipeline cache on disk, so a later start skips the work. The cache is generated data that is never
tracked or shipped as an asset. A missing, unreadable or mismatched cache (new driver, new engine) is ignored,
built again and rewritten, and never fails a start. The planner names the cache's place and the split of the
work.

## Reasoning
With the work on the main thread, the window cannot redraw or answer the OS until the work is done, so it looks
frozen and can show as not responding. The alternatives:
- A cache alone: it still freezes on the first start and after every driver or engine update.
- Splitting the work into slices between frames on the main thread: it needs fiddly cutting, and one long
  driver call still freezes the window.
- A general job system: much more than this needs.

One worker with a hand-off at the end keeps the world single-threaded, as everything else assumes (0249, 0256,
0284).

## Replaces
Nothing. Extends 0346. Takes the pipeline cache out of `ideas.md`.
