# 10 — A frame's point lights carry their bounces and bounce strength
folder: 3d
after: 01, 09
decisions: 0168, 0326

## Change
0326 point 1, in the point-light fill; nothing bounces a lamp yet (card 11).
- `3d/src/draw_point_lights.c`: each kept light's record gets the row's `bounces` and
  `bounce_strength`; a light with intensity 0 is still left out. Header phrase.
- `3d/include/3d/draw_system.h`: `voe_3d_draw_system_point_lights`'s comment says the two are
  copied as authored.
- `3d/tests/point_lights.c`: a lamp of bounces 2 and strength 0.5 frames with both; one of
  bounces 0 frames with 0. Entry in `3d/tests/tests.md` if its wording no longer covers it.

## Done when
`ctest --test-dir build/debug -R "^3d/point_lights$"` passes.
