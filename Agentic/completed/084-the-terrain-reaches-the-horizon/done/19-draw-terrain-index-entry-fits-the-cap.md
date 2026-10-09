# 19 — The draw_terrain.c index entry fits the cap
folder: 3d/tests
after: none
decisions: 0168

## Change
Doc only; no code changes.

`3d/tests/tests.md`: the entry `draw_terrain.c` is 306 characters, over the
300 cap. Shorten it to one sentence well under 300: a 4096 m landscape seen
level from 2 m up shows its far ridge against the sky and ground below the
horizon, within the node budget, nothing at fade 1, and the bounce's cast
within its own budget; skips without a graphics card. Drop the numeric
detail (16 nodes, still draws at 4).

`3d/tests/draw_terrain.c`: read its header comment only. It already states
each dropped case (the five claims); change it only if one is missing. Touch
no test code.

## Done when
`checks.sh --folder 3d/tests` reports no finding naming `draw_terrain.c`.
