# 63 — 3d relights the one-metre nest when a light blocker goes
folder: 3d
after: none
decisions: 0168, 0389, 0390

## Change
`3d/tests/bounce_scene.c` fails its BLOCKED check at line 566: the blocker destroyed and settled, the
patch reads `98 98 98 against 106 98 98`. Card 61 proved render relights a nest whole when its blocker
count changes, if begun directly (`render/tests/blocked_bounce.c`, `a_removed_blocker_relights_the_nest`,
passes). So what 3d begins the nest with, or when, is at fault. By 0389 point 5 a blocker change is a
lights change: every begun volume relights whole. Find and fix it in 3d.

- Find it. In `a_blocker_keeps_the_bounce_out` of `3d/tests/bounce_scene.c`, print each frame's
  `bounce relight` and `bounce sun shadow` entries from `voe_render_frame_pass_times` (its declaration
  in `render/include/render/device.h`), and whether `settles` saw a probing frame fail, from the blocker
  added until the second settle ends. Read with them the headers of `3d/src/draw_bounce.h`,
  `3d/src/draw_bounce.c`, `3d/src/draw_shadows.c` and `3d/src/draw_light_blockers.c`, and the comments
  on `voe_render_bounce_begin`, `_shadow_pass_begin` and `_relight` in `device.h`; then the body of
  `draw_bounce.c`. Candidates, none proven:
  - the nest's begin does not get the same blockers, masks and sun mask as volume 0's on every frame;
  - the nest's `corner` and `cell` disagree, so render's about-the-origin compare (corner − cell ×
    spacing) sees no change, or sees one at the wrong frame;
  - a shadows call that fails part-way (a probing frame's spent passes) begins volume 0 but not the
    nest, or the nest without its sun map or relight, and the frame after finds nothing changed.
- Fix it in `3d/src/draw_bounce.c` (or the 3d file the trail ends in). Take the prints out. Correct that
  file's header where it said otherwise, and its `3d/src/src.md` entry if what it says changed.
- If the trail ends in render (render drops a lights change for a volume whose begin, sun map or
  relight a failed call cut short), make no fix: block, naming the call sequence per frame and per
  volume, so a render card can repeat it in `blocked_bounce.c`.
- No test file or header change is needed beyond the prints, unless the fix changes what the BLOCKED
  paragraph says.

## Done when
`ctest --test-dir build/debug -R '^3d/bounce_scene$'` passes, with its BLOCKED check reading the patch
within 1/255 of before once the blocker is destroyed.
