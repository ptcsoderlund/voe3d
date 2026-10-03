# 35 — Two views in one frame each draw their bounce through the shadows call
folder: 3d/tests
after: 34
decisions: 0168, 0326, 0330

## Change
The standing test for bug 02 of 051, through the call the editor makes once per view, and the
human's proof in the editor. No code outside the test changes.
- `3d/tests/bounce.c`: a TWO VIEWS case on one device with a target: per frame, the shadows call
  (`voe_3d_draw_system_shadows`, `3d/include/3d/draw_system.h`) with `frame.target` the window, then
  again with a second frame whose `target` is the made target, its eye a few metres off, the sun
  casting at bounces 1. Frame one: both true. Frame two: true with exactly the passes both views
  open (each its four cascades and its bounce shadow pass; capture passes as render's frame budget
  of `VOE_RENDER_BOUNCE_CAPTURE_PASSES` gives them, `render/include/render/device.h`), false with one
  fewer. Count them from those headers' comments; the target's creation is in the same header's
  targets section. Header paragraph for the case, naming bug 02 and 0330.
- `3d/tests/tests.md`: the `bounce.c` entry says two views in one frame each draw their sun map.

## Done when
- `ctest --test-dir build/debug -R "^3d/bounce$"` passes.
- The human, on a hardware card: `voe_editor` (debug) started with no arguments opens
  `examples/tank_game` with more than one view; it does not abort, and every view shown draws and
  keeps drawing, with the sun's Bounces at 1 in each.
