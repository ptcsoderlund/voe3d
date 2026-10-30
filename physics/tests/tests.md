# tests

One plain C program per `physics` module, found by the build. None needs a
window or a graphics card.

- `body.c` — the default row, the slope limit's range of 0 to π/2 rad, a row
  replaced whole and kept over bad fields, and the move resting, at walls, on ramps, onto a ledge, off an edge, through
  a switched-off floor, and a switched-off body not moved.
- `collider.c` — registration, the kind as a named UINT32, a replace landing
  when the system runs and a negative size keeping the row, the shape of a
  scaled far box and a capsule, and none for a box switched off or under a
  switched-off parent.
- `overlap.c` — a capsule on a floor, at a wall and on a tilted box, round
  shapes against each other, triggers, `ignore` and capacity, a child found
  at its world place, and a switched-off floor not touched, near and far.
- `sweep.c` — a ray into a sphere, a sphere into a capsule, a miss, the nearer
  of two, `ignore`, triggers, capacity, a start inside, boxes: a turned face,
  a corner, an edge, a thin box, a child's box, a ray inside; a switched-off
  sphere not gathered; near and 1e6 m out.
