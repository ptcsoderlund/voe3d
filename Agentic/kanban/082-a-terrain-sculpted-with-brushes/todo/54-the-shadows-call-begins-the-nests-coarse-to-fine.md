# 54 — The shadows call begins the nests, coarse to fine
folder: 3d
after: 48, 52
decisions: 0168, 0386, 0387, 0388, 0389

## Change
0389 points 1 to 3: after the level grid, `voe_3d_draw_bounce` begins every nest finer than it.

- `3d/src/draw_bounce.c`:
  - Make one static helper out of what the level grid does now: begin, capture passes, one sun map per
    casting sun, relight. It takes the grid, the volume index and how many capture passes this call may
    have reached when it stops.
  - The level grid is volume 0, as now. n is 1 + the nests whose spacing (card 48) is below the level grid's.
  - For each such nest, coarse to fine: `voe_render_bounce_placed` for its volume (nest i is volume i + 1),
    then `voe_3d_bounce_grid_nest` with that cell (or NULL), then the helper.
  - Every volume gets the same lights, blockers and stale spheres.
  - The j-th volume begun (from 0) opens capture passes until this call's count reaches
    `VOE_RENDER_BOUNCE_CAPTURE_PASSES − (n − 1 − j)`, or render opens no more.
  - A nest's sun map is `voe_3d_bounce_grid_sun` of the nest's grid.
- `3d/src/draw_bounce.h`: the `voe_3d_draw_bounce` comment says the above in a sentence or two.
- `3d/include/3d/draw_system.h`, the shadows paragraph (near line 595–625):
  - The level grid is fitted to the still casters. Nests of 16, 4 and 1 m are placed about the eye, each
    only when finer than the level grid, and move only past two cells (0389).
  - Up to `VOE_RENDER_BOUNCE_CAPTURE_PASSES` capture passes in all, shared coarse to fine.
  - On a frame that relights a volume, one sun map per casting sun for that volume.
  - The device sizing sentence: `VOE_RENDER_BOUNCE_VOLUMES` × the sun maps.
  - The camera now moves the nests, and the level grid still never moves.
- `3d/tests/bounce.c`:
  - Its world (a 40 m ground) fits a 2 m level grid, so the 1 m nest also begins. Fix the pass counts the
    header states and the cases check. The four capture passes are now shared, and a relighting frame opens
    a sun map per volume. Size its device from `VOE_RENDER_BOUNCE_VOLUMES`.
  - New cases:
    - `a_two_metre_level_begins_only_the_one_metre_nest`
    - `the_finest_nest_keeps_a_capture_pass`: with the level grid still capturing, frame two opens one
      capture pass on volume 3.
- `3d/tests/tests.md`: the `bounce.c` entry matches.

Per frame (0388):
- `bounce capture N`: unchanged, at most 4.
- `bounce sun shadow`: one more pass per relighting nest per casting sun.
- `bounce relight`: one per begun volume, only for the probes that changed or fade.
- A still camera in a still world records none of them.

## Done when
`ctest --test-dir build/debug -R '^3d/bounce$'` passes with the two new cases.
