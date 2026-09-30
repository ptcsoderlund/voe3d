# 02 — A sphere sweeps against boxes
folder: physics
after: 01
decisions: 0168, 0293

## Change
0293 point 4's box. Read the headers of `include/physics/sweep.h` and
`src/sweep.c` (01), and `include/physics/shape.h`.

- `src/sweep.c`: a box obstacle is tested, no longer passed over. In the
  box's frame (the offset and motion turned by the inverse rotation): the
  slab test against the box grown by `radius` on every axis gives the entry
  `t` and face. Where the entry point lies outside the unscaled box on one
  axis only, that face is the hit. On two or three axes, the ray is tested
  against the capsules of radius `radius` on the box's edges meeting there
  (the three at the nearest corner), the least `t` wins, and none hit is a
  miss. The normal is the face's axis, or from the edge's closest point to
  the sweep centre, turned back to the world. A ray (radius 0) is the slab
  test alone. A sweep starting inside the grown box, but not outside the
  rounded one, is the start-overlapping answer of 01.
- `include/physics/sweep.h`: its header says boxes are exact rounded boxes,
  and no longer that they wait for 02.
- `tests/sweep.c`: a ray hits a box face turned 45° about Y at the expected
  point and normal; a swept sphere of 0.5 aimed so its path passes 0.6 m
  past a box's corner diagonally misses, where the sharp grown box would
  hit; one grazing an edge hits with the normal from the edge; a box 0.05 m
  thick is hit by a sphere of 0.05 swept 100 m through it in one sweep; a
  box on a scaled, turned parent is hit at its world place; a ray starting
  inside a box hits at 0.
- `src/src.md`, `tests/tests.md`: the box, each entry under 300 characters.

## Done when
`ctest --test-dir build/debug -R '^physics/sweep$'` passes, after the
folder's build.
