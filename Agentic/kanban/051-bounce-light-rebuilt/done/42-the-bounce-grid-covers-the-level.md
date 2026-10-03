# 42 — The bounce grid covers the level, not the camera
folder: 3d
after: 38, 41
decisions: 0168, 0326, 0329, 0331, 0332

## Change
0332 points 2 and 4, for bug 03 of 051: the grid stood on the eye, so things past about 24 m got no
bounce and moving the camera moved that edge. `voe_3d_bounce_grid_fit` is the one owner; its only
callers are in 3d. Since card 40, render asserts a begin's spacing of nought; this card sets it.
- `3d/include/3d/bounce_grid.h`: `voe_3d_bounce_grid` gains `float spacing`;
  `voe_3d_bounce_grid voe_3d_bounce_grid_fit(voe_math_double3 min, voe_math_double3 max, voe_math_double3 eye)`,
  min above max on any axis meaning no still caster. The rule is 0332 point 2: spacing
  `VOE_RENDER_BOUNCE_SPACING` × 2^k, k from 0 to at most 16, the smallest at which the grid whose
  lowest cell is the box centre's cell less 12, 6, 12 holds every cell of the box with one cell
  spare each side; no box: k 0 about the world origin. `corner` as now, at the spacing.
  `VOE_3D_BOUNCE_BELOW` goes. `voe_3d_bounce_grid_sun`'s sphere: half-diagonal plus the volume's
  reach, `VOE_RENDER_BOUNCE_REACH` × spacing / `VOE_RENDER_BOUNCE_SPACING`. Header rewritten: on
  the level, not the eye (0331); why powers of two; why a cell spare; the eye only in `corner`.
- `3d/src/bounce_grid.c`: to match.
- `3d/src/draw_bounce.h` and `.c`: `voe_3d_draw_bounce` takes card 38's box, fits the grid to it,
  sets the frame record's `spacing`; `voe_3d_bounce_stale` gains `float spacing`, the sphere's w the
  larger of `VOE_3D_BOUNCE_REACH` and 3 × spacing. Comments: fitted to the level, not the eye.
- `3d/include/3d/draw_system.h`: `VOE_3D_BOUNCE_REACH`'s comment (the least radius; 3 cells on a
  coarser grid) and the shadows call's bounce paragraph (the grid fitted to the still casters'
  box, not about `frame->eye`).
- `3d/tests/bounce_grid.c`, rewritten: the box (−20, −0.1, −20) to (20, 10, 20) fits at 2 m with
  `cell` (−12, −4, −12); the same box at eyes (0, 0, 0), (300, 5, −40) and 10 km out gives the
  same cell and spacing, `corner` the lowest corner about each; a 60 m wide box fits at 4 m and not
  at 2; a box whose centre crosses a cell edge moves `cell` by one; no box: 2 m about the origin;
  the sun view's sphere grows with the spacing. Header to match.
- `3d/tests/bounce.c`: the stale calls pass `VOE_RENDER_BOUNCE_SPACING` and its claims stand; one
  claim at spacing 8: w is 24. Header phrase.
- `3d/tests/bounce_scene.c`: the ground 40 m wide instead of 60, so the grid stays at 2 m as in
  the tank game; header's THE WORLD to match.
- Entries: `3d/include/3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md`: no "stands on the eye".

## Done when
`ctest --test-dir build/debug -R "^3d/(bounce|bounce_grid|bounce_scene)$"` passes and
`! grep -rn "VOE_3D_BOUNCE_BELOW" 3d` exits 0.
