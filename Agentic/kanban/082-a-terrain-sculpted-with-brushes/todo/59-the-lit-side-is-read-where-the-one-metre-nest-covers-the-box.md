# 59 — The lit side is read where the one-metre nest covers the box
folder: 3d
after: 58
decisions: 0168, 0389, 0390

## Change
Card 56's work is in the tree (commit a8842fcb): `3d/bounce`, `3d/shadow_lights` and every wait, budget
and case of `3d/bounce_scene` pass but the lit side (`2 * far < near`). Its eye, 8 m up and 14 m back
(+z), places the 1 m nest from z 2 m on, so the box at z 0.5 is read from the 2 m level grid alone.
Card 58 made the read never darken beside a box (0390). 0390 keeps the lit-side assertion and TINT as
they are, and has the test place its eye where the 1 m nest covers the box. Tests only; do not change
`3d/src` or `3d/include`.

- `3d/tests/bounce_scene.c`:
  - THE WORLD's eye moves so the 1 m nest holds the lit-side points (ground at z 0.5, x 1.75 and 4.75,
    read 0.25 m up) clear of its 2-cell edge band. By 0389 point 2 the nest's lowest cell is the eye's
    cell less (12, 9, 12), 24 × 12 × 24 cells; full weight needs the eye's cell z at most 10 and y at
    most 6. For example 5 m up and 8 m back, still looking at the origin.
  - Every other case keeps its assertion. Where a point it reads leaves the 128-pixel picture from the
    new eye, move that point into view and keep what it checks. MOVE and FAR stay relative to the eye.
  - Header: THE WORLD gives the new eye and why (0390: a small thing's colour is a near-camera detail,
    0387); any paragraph naming a point that moved matches.
- `3d/tests/tests.md`: the `bounce_scene.c` entry, if it names the eye.

If the lit side still fails with the eye so placed, block and give the near and far readings and where
the 1 m nest was placed (`voe_render_bounce_placed`).

## Done when
`ctest --test-dir build/debug -R '^3d/(shadow_lights|bounce_scene|bounce)$'` passes.
