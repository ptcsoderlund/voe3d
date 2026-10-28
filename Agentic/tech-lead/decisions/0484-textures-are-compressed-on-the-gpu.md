# 0484 — Textures are compressed on the GPU
date: 2026-10-28
by: tech-lead

## Decision
1. **Every BC7 and BC5 compression runs on the GPU,** in a compute encoder, wherever a picture is compressed:
   under the splash, in the editor between frames, in the cook before Play and Ship, and in the game's first
   load. The compressed blocks are read back and written to the same cached copies as now (0470).
2. **The CPU encoder goes.** There is one encoder, not a GPU one and a CPU fallback. A cook with no window
   makes a headless device to compress on. Every card in 0318's range has compute.
3. **Quality is at least today's.** It matches today's BC7 mode 6 and BC5. The encoder may search more BC7
   modes if that looks better and stays fast; the planner decides.
4. **A picture is compressed once per change.** A picture that both a model's copy and a material's map hold is
   not compressed twice.
5. **The bar.** A project with a few models and textures, Cache folder deleted, opens in the editor's debug build
   in under 30 seconds. A changed map is compressed within a second, with no stutter you can see.
6. **It is built next,** as work order 098 (unused since 0479), so `/drive` takes it before 100–103.

## Reasoning
0477 chose a CPU encoder on one thread, paced at 2 ms a frame, expecting "a few seconds of waiting". It takes 15
minutes and more for a small project. A GPU encodes every block of a texture at once. 0477 rejected it for a
readback, a large shader and a cook with no device. Under 0483 the first two are no reason. The third is solved
by a headless device. Alternatives: a faster CPU encoder (picking levels directly, optimised even in debug),
with worker threads under the splash. That is rejected: it stays slower, scales badly with 4K maps and with the
hill's many materials, and reverses 0456's one thread.

## Replaces
0477 points 2 and 4 (the encoder, and one thread) and the GPU line of its Reasoning. 0477 point 3's 2 ms pacing
stays only as the planner sees fit for the GPU queue. Also amends 0479's queue: 098 comes before 100.
