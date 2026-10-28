# 0489 — The editor's queue waits on the GPU, and a queued map loads empty
date: 2026-10-28
by: planner

## Decision
0477 point 3's queue (`game/compressing.h`) on the GPU (0484 point 1):

1. **A queued map is not read on the caller.** With a queue, a map with no fresh copy is added by
   path, kind and stamp, and its material loads as if that map were empty until the copy is
   written and the materials naming it load again. No RGBA8 upload outside a splash.
2. **A step never waits.** The oldest job moves through: read, decode and mip by kind on a
   one-worker burst (0487); `voe_render_compress_start`; `_ready` polled each step; `_finish`;
   the copy written on a one-worker burst; its path handed back. One job is on the GPU at a time.
3. **0477's 2 ms pacing is dropped**: nothing in a step runs long. At most 16 jobs wait, as now.

## Reasoning
Decoding a large PNG and uploading it uncompressed on the main thread is a visible hitch, and
0484 point 5 forbids one. With the GPU compressing in milliseconds the copy follows within about
the decode's time, so showing the old empty slot for that long beats a stutter and a second upload.

## Replaces
0477 point 3's uncompressed upload and its 2 ms pacing, as 0484's Replaces allows.
