# 12 — The bounce as the editor and the game draw it, through 3d's calls
folder: 3d
after: 11
decisions: 0168, 0312, 0326

## Change
How to test steps 2, 3 and 8 as a test that stays in the tree. Rewrite `3d/tests/bounce_scene.c`
(046's bug 01 test; its claims are 0310's and go) on the same calls the game and the editor
make: `voe_3d_draw_system_frame`, `_point_lights`, `_shadows`, a window pass, `_run`, read back.
- One world: a camera above and behind looking down at grey ground, a strongly red box in
  sunlight, the sun casting with bounces 1 and strength 1, shapes casting. Draw frames until a
  shadows call opens no capture pass (count passes as `3d/tests/bounce.c` does after card 11),
  with a bound of frames that fails the test when it is reached.
- Claims: the ground at the box's lit side reads redder than at bounces 0, the redness fading by
  3 m off; the ground in the box's shadow at its foot within 4/255 of bounces 0; open ground at
  five points 2 m apart within 2/255 of each other, and again after the sun turns by 5° (each
  point's brightness changed with the sun, the five still even).
- Settling: with nothing changed the next frame opens no capture pass. The box moved 1 m by its
  transform with a previous table (as `bounce.c` marks stale spheres): the next frame opens a
  capture pass, and within the bound it settles again.
- Header: what the world is, why each claim, which How to test step it stands for; skips with a
  reason without a graphics card, as now. Entry in `3d/tests/tests.md`.

## Done when
`ctest --test-dir build/debug -R "^3d/bounce_scene$"` passes.
