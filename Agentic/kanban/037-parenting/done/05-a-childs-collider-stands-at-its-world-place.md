# 05 — A child's collider stands at its world place
folder: physics
decisions: 0168, 0281, 0253

## Change
Needs card 03.

- `physics/src/shape.c`: the collider's shape is built from
  `voe_scene_transform_world(world, entity)` instead of the row (`voe_scene_transform_get` stays
  only as the "has a transform" test). Its header comment says the shape stands at the world
  place, a parent's included.
- `physics/src/body_move.c` is not changed: a body moves its own row. Add one line to its header
  comment that a body is expected to be a root, because its move is written as its row.
- `physics/tests/overlap.c`: a new case — register `voe_scene_parent_register`, a box collider
  2 m along +X under a parent at (10, 0, 0); a sphere overlap at (12, 0, 0) finds it and one at
  (2, 0, 0) does not (raise the test world's `component_types` if needed). List the case in `physics/tests/tests.md`'s `overlap.c` line.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_physics $(ninja -C build/debug
-t targets all | grep -oE "^voe_test_physics_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R
"^physics/"` exits 0, `voe_test_physics_overlap` among them.
