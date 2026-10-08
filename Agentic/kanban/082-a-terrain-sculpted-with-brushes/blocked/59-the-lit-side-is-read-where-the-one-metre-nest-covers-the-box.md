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

## Blocked
With the eye at (0, 5, 8) (open ground moved to x -6, z -12 to -4 to stay in the picture; FAR's pitch now
taken from the eye) the lit side passes: near 110 94 94 against 94 (16/255), far 97 95 95 (2/255). BLOCKED
then fails at its last check (bounce_scene.c, `unblocked patch`): once the blocker is destroyed and settled
(1 pair), the patch under it reads 98 98 98 against 106 98 98 tinted before, and still does 200 frames
later, so the 1 m nest that now covers the patch never relights the tint back after a blocker goes; with
the old eye (8 up, 14 back) and card 58 in the tree, every case passes, including the lit side (100 94 94,
far 96 94 94) and the unblocked patch (117 vs 117). Unblock by a render card for the nest's relight after a
blocker is removed, or by a decision to keep the old eye now that 0390 makes it pass.
