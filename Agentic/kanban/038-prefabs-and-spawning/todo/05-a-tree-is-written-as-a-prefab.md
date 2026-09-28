# 05 — A tree is written as a prefab
folder: authoring
decisions: 0168, 0283, 0281

## Change
Needs card 04. 0283 point 1: the text a new `.prefab` file holds.

- `authoring/include/authoring/prefab.h` (new): `[[nodiscard]] bool voe_authoring_prefab_write(
  const voe_ecs_world *world, voe_ecs_entity root, voe_base_arena *arena,
  voe_authoring_text *out);` — the scene text of `root` and everything under it
  (`voe_scene_parent_tree`, scene/parent_component.h), in scene_write.h's order and spelling,
  ids as they are in the world; the root's parent section left out; the root's transform written
  at the origin with no turn and scale one; an ENTITY naming outside the tree written `0` with
  no warning; no kept sections. Refuses, reported naming the entity, a root that is not alive or
  has no identity, and anything scene_write refuses. Same arena contract as scene_write.
  Header points: what a prefab file is (0283 point 1, one tree, ids local to the file); that
  the root's transform in the file is where it sits while open and nothing else; that what a
  prefab may hold (no camera, light, prefab or part) is checked by the caller before it writes
  and by the reader (card 06) when it reads.
- `authoring/src/prefab_write.c` (new): reuse the walk in `authoring/src/scene_write.c` by giving
  it the set of entities to write and the root whose parent and transform are overridden — a
  private parameter or struct in a private header, not a second copy of the walk. Keep
  `voe_authoring_scene_write`'s behaviour byte for byte.
- `authoring/tests/prefab.c` (new): a hull with a turret under it and a barrel under that, plus
  an unrelated thing whose ENTITY field is not in the tree, and a hull placed at (3, 0, 5) turned
  and parented under something: the text holds the three, the hull's transform at the origin
  with scale one, no parent section on the hull, the turret's and barrel's parent ids as in the
  world, nothing of the unrelated thing; a dead root is refused.
- `authoring/authoring.md`, `authoring/include/authoring/authoring.md`, `authoring/src/src.md`,
  `authoring/tests/tests.md`: the new files.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_authoring $(ninja -C build/debug
-t targets all | grep -oE "^voe_test_authoring_[A-Za-z0-9_]+") && ctest --test-dir build/debug
-R "^authoring/"` exits 0, `voe_test_authoring_prefab` and `voe_test_authoring_scene_write`
among them.
