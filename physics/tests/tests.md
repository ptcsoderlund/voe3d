# tests

One plain C program per `physics` module, found by the build. None needs a
window or a graphics card.

- `body.c` — the default row, a row replaced whole and kept over bad fields,
  and the move resting, at walls, on ramps, onto a ledge and off an edge.
- `collider.c` — what registration tells a tool, the kind field described as a
  named UINT32, that a replace lands when the system runs and a negative size
  keeps the row, and the shape a scaled box far from the origin and a capsule
  become.
- `overlap.c` — a capsule on a floor, at a wall and on a tilted box, round
  shapes against each other, triggers, `ignore` and capacity, and a child
  found at its world place, not its row's, near and far.
