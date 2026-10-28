# 0486 — render compresses a mipped chain in a compute encoder, on the program's own device
date: 2026-10-28
by: planner

## Decision
Feature 098 carries out 0484 so:

1. **The encoder is `render`'s**, three calls in `render/device.h`: `voe_render_compress_start`
   (a caller's RGBA8 chain, level by level as `voe_render_texture_create_levels` takes it, into
   BC7 or BC5 blocks as `voe_render_texture_create_blocks` takes them), `_ready` (no wait) and
   `_finish` (waits if needed, copies the blocks out or drops them, frees the job). A blocking
   caller is start then finish.
2. **One compute thread a 4×4 block, one dispatch a level**, in `render/shaders/compress.slang`
   (entry points `bc7` and `bc5`). The chain goes into one host-visible storage buffer and the
   blocks come out of another, read back directly; no staging copies, no image. An edge block
   repeats the nearest edge texel. The pipelines are built at the first start and torn down at
   close.
3. **Quality is today's, block for block.** BC7 is mode 6 as `assets/src/image_bc7.c` does it
   (principal axis, both p-bits tried, nearest indices, one least-squares refit), and BC5 is two
   BC4 halves with both of BC4's modes tried, as `image_bc5.c` does. Other BC7 modes wait for a
   look that asks for them (0484 point 3 leaves it to the planner).
4. **The chain is mipped on the CPU by use** (`voe_assets_image_mips`, 0466), as copies are now.
   The GPU does not mip here: a copy's levels must be the by-use average.
5. **Threads.** The three calls join the calls the ONE other thread may make (0370 point 5). The
   guard is held to record and submit and to free, never through the wait.
6. **A cook compresses on the program's own device.** Every cook today runs in a program with a
   window: the editor's prepare and its cook before Play and Ship on the splash worker, the
   game's first load on its own. 0484 point 2's headless device is made only by a cook with no
   window, and none exists; the tests compress on `voe_render_device_new_headless`.

## Reasoning
Porting the CPU encoder's search keeps 0484 point 3 measurable: the tests that held the CPU
encoder to its bounds hold the GPU one to the same. Host-visible buffers keep one submission per
job and no copy; a 4096 chain is about 90 MB in and 22 MB out, freed at finish. Mipping stays on
the CPU because it is by use and already runs in the decode burst (0487). A second device in a
program that already has one would double the memory and the pipelines for nothing.

## Replaces
Nothing; reads 0484 point 2 as above.
