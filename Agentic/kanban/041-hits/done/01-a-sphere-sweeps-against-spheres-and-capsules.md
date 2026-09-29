# 01 — A sphere sweeps against spheres and capsules
folder: physics
after: none
decisions: 0168, 0293

## Change
0293 points 1–5, but for boxes, which 02 adds. Read the headers of
`include/physics/overlap.h`, `include/physics/shape.h` and `src/overlap.c`
(its float-about-the-query's-centre pattern and quaternion rotate).

- `include/physics/sweep.h`, new:
  - `voe_physics_obstacle`: `entity`, `voe_physics_shape shape`, `float reach`
    (the bounding radius about the shape's centre: a sphere `half.x`, a
    capsule `half.y`, a box the length of `half`).
  - `voe_physics_hit`: `entity`, `float t`, `voe_math_double3 point`,
    `voe_math_float3 normal`.
  - `uint32_t voe_physics_obstacles_gather(const voe_ecs_world *world,
    voe_physics_obstacle *out, uint32_t capacity)`: every collider that is
    not a trigger and has a shape (`voe_physics_shape_of`), in table order;
    returns how many were written, past `capacity` dropped.
  - `[[nodiscard]] bool voe_physics_sweep(const voe_physics_obstacle
    *obstacles, uint32_t count, voe_math_double3 from, voe_math_float3
    motion, float radius, voe_ecs_entity ignore, voe_physics_hit *out)`:
    the first obstacle touched, as 0293 point 3 says; false and `*out` left
    alone when none. Asserts a radius at or above 0 and finite motion.
  - Header points: a ray is radius 0; why the caller gathers and why that is
    no cache (0249 rule 1); triggers are not gathered; float about `from`;
    the start-overlapping answer; the reach reject; boxes (02).
- `src/sweep.c`, new: the gather; the sweep as the segment `from` to
  `from + motion`, obstacles' centres minus `from` in double, then float.
  Per obstacle: the reach reject (segment-to-centre distance over reach plus
  radius), then the exact test: a sphere as a ray against a sphere of the two
  radii; a capsule, in its own frame, as a ray against the infinite cylinder
  of the two radii clipped to its segment, then the two cap spheres; a box
  passed over until 02. The least `t` wins; `point` is the sweep centre at
  `t` moved `radius` against the normal, back in double.
- `tests/sweep.c`, new, one plain C program: a ray hits a sphere at the
  expected `t`, point and normal; a swept sphere hits a standing capsule on
  its side and, from above, on its cap; a sweep passing beside misses; the
  nearer of two obstacles wins; `ignore` skips one; a trigger is not
  gathered; a sweep starting inside hits at 0 against the motion; a scene
  1e6 m from the origin gives the same hit as at it, within 1e-4 m;
  capacity drops the rest. Build worlds as `tests/overlap.c` does.
- `physics.md` (a `sweep.h` line), `src/src.md`, `tests/tests.md`: the new
  files, each entry under 300 characters.

## Done when
`ctest --test-dir build/debug -R '^physics/sweep$'` passes, after the
folder's build.
