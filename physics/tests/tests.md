# tests

One plain C program per `physics` module, found by the build. None needs a
window or a graphics card.

- `body.c` — the default row and the collider it needs, a row replaced whole,
  and each bad step, slope and velocity keeping it.
- `collider.c` — what registration tells a tool, that a replace lands when the
  system runs and a negative size keeps the row, and the shape a scaled box far
  from the origin and a capsule become.
- `overlap.c` — a capsule on a floor, at a wall and on a tilted box, round
  shapes against each other, triggers, `ignore` and capacity, near and far.
