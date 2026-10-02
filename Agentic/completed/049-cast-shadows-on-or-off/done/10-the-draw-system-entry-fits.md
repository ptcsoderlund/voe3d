# 10 — The draw system's index entry fits its cap
folder: 3d/include/3d
after: none
decisions: 0168

## Change
`3d/include/3d/3d.md`: the entry for `draw_system.h` is 328 characters; the
index cap is 300. Cut it to one sentence: the system draws the world into the
pass the loop opened, after the sun's shadow and bounce passes. The detail it
drops (only a light that casts, only what casts, only when the sun bounces)
belongs in the header of `3d/include/3d/draw_system.h`; open its header comment
and add that point there if it is not already said. No code changes.

## Done when
`checks.sh --folder 3d/include/3d` reports no finding for `3d.md`.
