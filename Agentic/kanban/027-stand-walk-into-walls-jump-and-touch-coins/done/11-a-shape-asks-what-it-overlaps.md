# 11 — A shape asks what it overlaps
folder: physics
decisions: 0168, 0249, 0253, 0250

## Change
0253 point 3: the one query. It only reads (0249 rule 1).

- `physics/include/physics/overlap.h` (new) — `voe_physics_contact { voe_ecs_entity entity;
  voe_math_float3 normal; float depth; bool trigger; }` and `uint32_t voe_physics_overlap(const
  voe_ecs_world *world, voe_physics_shape query, voe_ecs_entity ignore, voe_physics_contact *out,
  uint32_t capacity)`: every collider (its `voe_physics_shape_of`) the query overlaps by more than
  nothing, except `ignore`'s; `normal` is unit, the way to push the query out; `depth` how far.
  Returns how many were written; past `capacity` the rest are dropped. A box query asserts (no
  caller, rule 10). Header points: float about the query's centre, each other shape's centre
  subtracted in double first (0250); a sphere is a capsule with no segment; why ray and sweep
  are not here yet.
- `physics/src/overlap.c` (new) — the query as a segment plus radius, in float about its centre:
  against a sphere, the closest point on the segment; against a capsule, the closest points of two
  segments; against a box, the segment taken into the box's frame, the closest point pair found
  by a bounded search along the segment (the distance to the box is convex along it), and when
  the segment's point is inside the box, out along the face of least depth. `physics/src/src.md`.
- `physics/tests/overlap.c` (new) — a capsule standing on a box floor sunk 1 cm: one contact,
  normal (0, 1, 0), depth 0.01 ±1e-5; against a wall box: normal horizontal; against a box
  rotated 20° about Z: normal along its face; sphere-sphere and capsule-capsule; a trigger's
  contact flagged; `ignore` left out; capacity 1 with two contacts writes one; the same scene
  moved by (100000, 0, 100000) gives the same contacts to 1e-5. `physics/tests/tests.md`.
- `physics/physics.md` — the entry for `overlap.h`.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder physics` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R '^physics/overlap$'` passes.
