# 10 — A physics folder holds colliders
folder: physics
decisions: 0168, 0175, 0249, 0253, 0250

## Change
`physics/` does not exist; this card makes it (0253 point 1) and registers it (0175): the
`physics` row `scene ecs math base` in `cmake/voe.cmake` with a comment naming 0253, and
`add_subdirectory(physics)` after `scene` in the root `CMakeLists.txt`. The four-line
`physics/CMakeLists.txt` says `voe_module(physics DEPENDS scene ecs math base)`. Model every file on
`scene`'s transform and light pair (`scene/include/scene/light_component.h`,
`light_system.h`) and `3d/include/3d/shape_component.h` for named enum values.

- `physics/include/physics/collider_component.h` — `voe_physics_collider` described: `kind` ENUM
  (`VOE_PHYSICS_COLLIDER_BOX` 1, `_SPHERE` 2, `_CAPSULE` 3, 0 none), named "Box", "Sphere",
  "Capsule" for a dropdown; `size` FLOAT3; `trigger` BOOL. Key `voe_physics_collider_key` and
  the names, each `extern VOE_BASE_IMPORTED` (0245); `_get`, `_count`, `_rows`, `_entities`. Header points: 0253
  point 2's meaning of `size` per kind and that the transform's scale applies; plain data any
  reader may use (0249 rule 4); a trigger blocks nothing.
- `physics/include/physics/collider_system.h` — `voe_physics_collider_register(world, capacity)`
  (table, description, whole-row replace intent, default a box of 1 not a trigger, needs a
  transform, menu "Physics / Collider"); `_add`; `voe_physics_collider_intent` and `_submit`;
  `voe_physics_collider_system_run` drains: a size not finite or below 0 keeps the row, reported
  as `scene/transform_system.h` reports; an unknown kind is kept and warned.
- `physics/include/physics/shape.h` — `voe_physics_shape { uint32_t kind; voe_math_double3
  centre; voe_math_quat rotation; voe_math_float3 half; }` — a collider in the world: box half
  extents; sphere radius in `half.x`; capsule radius in `half.x` and half its whole height in
  `half.y` (never below the radius). `voe_physics_shape_of(world, entity, voe_physics_shape
  *out)`: false with no collider, no transform or an unknown kind. A sphere's radius uses the
  largest |scale|; a capsule's radius the larger of |x| and |z|, its height |y|.
- `physics/src/*.c` for the three, `physics/src/src.md`; `physics/physics.md` in the shape of
  `scene/scene.md`.
- `physics/tests/collider.c` — register, add, replace drained, a negative size kept; shape_of a
  box scaled (10, 0.1, 10) at (100000, 0, 0) gives half (5, 0.05, 5) and the exact centre; a
  capsule (1, 2, 1) at scale 1 gives radius 0.5, half height 1. `physics/tests/tests.md`.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder physics` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R '^physics/collider$'` passes.
