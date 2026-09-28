# 03 — Parenting and unparenting keep the world place, and a lag blends the tree
folder: scene
decisions: 0168, 0281, 0271, 0190, 0254

## Change
Needs card 02. The one door every parenting goes through (0281 point 4), and the draw's blend
made a world transform (0281 point 2).

- `scene/include/scene/parent_system.h`, `scene/src/parent_system.c`:
  `[[nodiscard]] bool voe_scene_parent_set(voe_ecs_world *, voe_ecs_entity child,
  voe_ecs_entity parent);` — queues, in this order, a structural remove of the child's parent
  row, then when `parent` is not zeroed a structural add of `{ parent }`, then a transform
  intent whose row is `voe_scene_transform_relative(world(parent), world(child))`, or
  `world(child)` for a zeroed parent. False when a queue is full; what was already queued stands.
  Asserts: the table is registered, the child has a transform, a non-zeroed parent is alive
  with a transform, and the parent is not the child or under it (`voe_scene_parent_within`).
  Header points: remove-then-add works because the queue applies in submission order
  (ecs/structure.h); the world place is read at the submit, so the caller submits no other move
  of that child in the same frame; game code parents while playing through this (0271); the
  loop assert and why callers ask `_within` first.
- `scene/include/scene/transform_system.h`, `scene/src/transform_previous.c`:
  `voe_scene_transform_between` now blends each link of the chain by `lag` (as today, per row)
  and composes them, so it hands back a world transform; lag 0 equals
  `voe_scene_transform_world`. Update its comment and the previous-table paragraph to say the
  draw gets a world transform; the signature is unchanged, so its callers in `3d` need no edit.
- `scene/tests/parent.c`: a turret parented onto a moved and turned hull through `_set`, then
  `voe_ecs_structure_apply` and `voe_scene_transform_system_run`, has the same `_world` as before
  and the expected relative row; unparenting it keeps `_world` too and leaves no parent row;
  re-parenting from one hull to another works in one frame.
- `scene/tests/transform.c`: with the previous table registered, a hull moved one step with a
  turret on it — `_between(turret, 0.5)` is the turret's place halfway, and `_between(…, 0)` is
  `_world`.
- `scene/tests/tests.md`: both lines updated.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_scene $(ninja -C build/debug
-t targets all | grep -oE "^voe_test_scene_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R
"^scene/"` exits 0, `voe_test_scene_parent` and `voe_test_scene_transform` among them.
