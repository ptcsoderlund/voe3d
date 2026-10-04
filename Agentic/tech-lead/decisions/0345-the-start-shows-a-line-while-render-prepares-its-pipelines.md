# 0345 — The start shows a line while render prepares its pipelines
date: 2026-10-04
by: planner

## Decision
For 055:
1. `render` opens a device with the pipeline layout and the element pipeline only. The five mesh
   pipelines (solid, blended, shadow, point shadow, capture) and the relight's startup are built by
   `voe_render_device_prepare`, one per call, until it answers prepared. Any pass that can use one
   (a pass with a camera, the shadow, point-shadow, bounce-capture and bounce-shadow passes, a
   bounce begin) first builds all that are left, so every caller that never prepares is unchanged.
2. `game/starting.h` draws one starting frame — the theme's ground over the whole surface, one line
   centred — polling the window but skipping the app's pace, and runs the prepare loop with a
   starting frame before each step. The game's run and the editor both use it.
3. `app/start_log.h` times a start's named steps from the moment the OS started the process
   (`voe_platform_clock_launched`, Linux `/proc/self/stat`) to the first normal frame, one line
   each and a total on stderr; the editor also appends the block to `start.log` in the engine's
   settings folder, emptied first when over 64 KiB. The game writes stderr only.
4. Linux only (0339): no Windows code is written; the feature's Windows steps are dropped.
5. No pipeline cache of our own: the driver's disk cache already serves a second start, and the
   log shows whether one is worth a later card.

## Reasoning
Text needs only the element pipeline and the font, so a line can be on screen before the costly
pipelines exist; building one per frame keeps the window presenting between them without threads.
Building the rest lazily on first need keeps `dev`, the tests and `--capture` untouched. On Wayland
a window shows nothing until its first present, so the first starting frame is the first thing
seen. The starting frame lives in `game` because the editor already names it and both programs
need the same frame. Alternatives: a background thread for pipelines (no threading in the engine);
a CPU-drawn splash copied into the swapchain (a second drawing path).

## Replaces
Nothing.
