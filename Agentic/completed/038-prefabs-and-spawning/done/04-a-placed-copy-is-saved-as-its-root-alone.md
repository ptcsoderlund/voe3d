# 04 — A placed copy is saved as its root alone
folder: authoring
decisions: 0168, 0283

## Change
Needs cards 01 and 03. 0283 point 3.

- `authoring/src/scene_write.c` (the walk, after card 03): an entity with a
  `voe_scene_prefab_part` row (scene/prefab_component.h) whose `instance` is another entity is
  not written at all, kept sections of its id included. One whose part row names itself is a
  placed copy's root: it is written with its identity, and of its component sections only
  `voe_scene_transform`, `voe_scene_parent` and `voe_scene_prefab`; its kept sections as today.
  A world with no part table registered writes as today. An ENTITY field elsewhere naming a
  skipped part is written `0` with the existing "target will not be in the file" warning.
- `authoring/include/authoring/scene_write.h`: the "what is written" list gains the two rules
  and why (a copy takes its parts from its prefab file at every read).
- `authoring/include/authoring/scene_cook.h`: one line saying the cook does not skip parts: it
  cooks the world as the editor holds it, copies expanded (0283 point 3). No code change there.
- `authoring/tests/scene_write.c`: a world made with `voe_game_world_new` is not available here
  (authoring does not see `game`); register what the test needs by hand as the file's other
  cases do, plus `voe_scene_prefab_register`. A root with a transform, a shape, a prefab row and
  a part row naming itself, and a child with an identity, a parent and a part row naming the
  root: the text holds the root's `[N]`, its transform and prefab sections and not its shape,
  and nothing of the child. The same world without part rows writes both whole.
- `authoring/tests/tests.md`: the line for `scene_write.c`.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_authoring $(ninja -C build/debug
-t targets all | grep -oE "^voe_test_authoring_[A-Za-z0-9_]+") && ctest --test-dir build/debug
-R "^authoring/"` exits 0, `voe_test_authoring_scene_write` among them.
