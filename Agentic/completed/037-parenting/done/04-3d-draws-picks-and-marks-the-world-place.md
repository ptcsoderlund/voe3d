# 04 — 3d draws, picks, outlines and marks a child at its world place
folder: 3d
decisions: 0168, 0281

## Change
Needs card 03. Meshes, models, panels, shadows and the camera already read
`voe_scene_transform_between`, which card 03 made a world transform. What is left reads the row
through `voe_scene_transform_get` and uses it as a place; each such use becomes
`voe_scene_transform_world(world, entity)` (keep the `_get` only as the "has a transform" test).

- `3d/src/draw_system.c`: the sun's direction (`turn`, near the top) from the world rotation.
- `3d/src/draw_marks.c`: the camera marker's pose, the sun marker's pose and the gizmo's
  transform.
- `3d/src/outline.c`: the outlined entity's transform.
- `3d/src/pick.c`: the four walks — shapes, models, camera markers, sun markers.
- Header comments of those files and `3d/include/3d/pick.h`, `3d/include/3d/outline.h`: where
  they say "its transform" as a place, say its world place (one phrase each; no new paragraph).
- `3d/tests/pick.c`: a new case — register `voe_scene_parent_register` in the test world, put a
  unit cube shape 5 m along +X under a parent at (0, 0, −10) with no shape of its own, and a ray
  down −Z from (5, 0, 0) hits the child; with the parent moved 3 m along +Y (through its transform
  intent and the system run) the same ray misses and one from (5, 3, 0) hits.
- `3d/tests/outline.c`: a child's outline quads lie about its world place (one case, same set-up).
- `3d/tests/tests.md`: the two lines mention a child. Raise a test world's `component_types` if it has no room for the parent.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_3d $(ninja -C build/debug -t
targets all | grep -oE "^voe_test_3d_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R "^3d/"`
exits 0, `voe_test_3d_pick` and `voe_test_3d_outline` among them.
