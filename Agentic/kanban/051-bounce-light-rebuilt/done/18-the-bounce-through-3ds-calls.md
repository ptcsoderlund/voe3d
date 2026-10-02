# 18 — The bounce as the editor and the game draw it, through 3d's calls
folder: 3d
after: 11, 17
decisions: 0168, 0312, 0326, 0327

## Change
How to test steps 2, 3 and 8 as a test that stays in the tree. `3d/tests/bounce_scene.c` was
rewritten by the blocked card 12 and is in the tree; it failed only step 3's claim, which card 17
fixes in render. Run it, and finish it if anything is missing from this list:
- The world: camera above and behind looking down at grey ground, a strongly red box in sunlight,
  the sun casting with bounces 1 and strength 1, shapes casting; drawn through
  `voe_3d_draw_system_frame`, `_point_lights`, `_shadows`, a window pass, `_run`, read back,
  until a shadows call opens no capture pass, with a bound of frames that fails the test.
- Claims: the ground at the box's lit side redder than at bounces 0, fading by 3 m off; the ground
  in the box's shadow at its foot within 4/255 of bounces 0; open ground at five points 2 m apart
  within 2/255 of each other, and again after the sun turns by 5° (each point changed with the
  sun, the five still even).
- Settling: nothing changed, the next frame opens no capture pass; the box moved 1 m by its
  transform with a previous table: the next frame opens one, and it settles again within the bound.
- Header: the world, why each claim, which How to test step it stands for, and for step 3 that it
  holds because of 0327; skips with a reason without a graphics card. Entry in `3d/tests/tests.md`.

If the shadow claim still fails, do not loosen it: block with the values read.

## Done when
`ctest --test-dir build/debug -R "^3d/bounce_scene$"` passes.
