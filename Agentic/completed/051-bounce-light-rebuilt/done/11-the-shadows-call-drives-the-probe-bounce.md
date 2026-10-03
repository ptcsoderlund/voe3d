# 11 — The shadows call drives the probe bounce for every light that bounces
folder: 3d
after: 10
decisions: 0168, 0316, 0326

## Change
0326 points 2 and 8. 046's bounce pass and update are no longer called; render removes them in
cards 13 and 14.
- `3d/include/3d/bounce_grid.h` and `3d/src/bounce_grid.c`: `voe_3d_bounce_grid` is `cell` and
  `corner` only; `voe_3d_bounce_grid_fit(view, eye)` loses the direction and the light view:
  centre eye + forward × `VOE_3D_BOUNCE_AHEAD` 24 in double, its cell at
  `VOE_RENDER_BOUNCE_SPACING`, less 12, 6 and 12 cells (`VOE_RENDER_BOUNCE_PROBES_XZ`/`_Y` halves)
  for the lowest; `corner` that cell's lowest corner about the eye. Header rewritten to match;
  `light_box.h` is no longer included here.
- `3d/src/draw_bounce.h` and `.c`: `voe_3d_draw_bounce` fits the grid, takes this step's stale
  spheres (as now), fills a `struct voe_render_bounce_frame` — the frame's light; the light row's
  `bounces` and `bounce_strength`, nought with no row; `frame->shadow`; `frame->points` — calls
  `voe_render_bounce_begin` on `frame->target`, then opens capture passes while
  `voe_render_bounce_capture_pass_begin` says opened, drawing `voe_3d_draw_casters` into each
  and closing it, then `voe_render_bounce_relight`. False when a pass or a draw is refused.
  Headers say so.
- `3d/src/draw_shadows.c`: the old bounce call after the cascades goes; after the point-shadow
  pass, the bounce when the frame is not blind and either the light row bounces with intensity
  above nought and not `unshaded`, or a light in `frame->points` has `bounces` ≥ 1 — whether or
  not the sun casts. Header: who bounces, and that nothing bounces costs nothing (0316).
- `3d/include/3d/draw_system.h`: `voe_3d_draw_system_shadows`'s bounce paragraph: when it runs,
  up to `VOE_RENDER_BOUNCE_CAPTURE_PASSES` more passes and that many more objects per caster,
  the first frame after a light starts bouncing builds the volume and shows none;
  `voe_3d_frame.target`'s and `VOE_3D_BOUNCE_REACH`'s comments name the volume, not 0308.
- `3d/tests/bounce_grid.c`: the centre 24 m ahead in whole cells, the corner 10 km out, 2 m
  along x one cell; the light-box claims go. Needs no card.
- `3d/tests/bounce.c`: pass counts: sun bounces 1, frame one opens no capture pass (the volume
  is wanted) and frame two four (true with exactly that many `passes`, false with one fewer);
  bounces 0 and no lamp that bounces: none, true on the cascades alone; a lamp of bounces 1 under
  a sun of 0 bounces too. The stale-sphere cases stay. Header rewritten.
- `3d/tests/shadows.c`: its sun bounces 0 (the test is about shadows); header phrase.
- `3d/src/src.md`, `3d/tests/tests.md`, `3d/3d.md`: entries for the files above.

## Done when
`ctest --test-dir build/debug -R "^3d/(bounce|bounce_grid|shadows)$"` passes and
`! grep -rn "voe_render_bounce_update\|voe_render_bounce_pass_begin" 3d` exits 0.
