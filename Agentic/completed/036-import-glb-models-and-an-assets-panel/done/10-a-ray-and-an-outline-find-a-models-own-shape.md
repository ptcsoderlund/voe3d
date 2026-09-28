# 10 — A ray and an outline find a model's own shape
folder: 3d
decisions: 0168, 0277, 0202, 0203

## Change
Needs card 07. `voe_3d_pick`'s signature changes; its caller in `editor` is card 14's, not
this card's (0277 point 3).

- `3d/include/3d/pick.h` and `3d/src/pick.c`: `voe_3d_pick(const voe_ecs_world *world, const
  voe_3d_shape_geometries *geometries, const voe_3d_models *models, voe_3d_ray ray,
  float *distance)`. After the shapes, a walk over model rows with a transform and a loaded
  entry tests the entry's `shape` triangles exactly as a shape's are tested; NULL walks none.
  Rewrite the "IT WALKS THE SHAPES" paragraph (the mesh table's turn has come as models) and the
  usage example.
- `3d/include/3d/outline.h` and `3d/src/outline.c`: `voe_3d_outlined` gains `const
  voe_3d_models *models`; an entity with no shape but a model whose entry is loaded is outlined
  from the entry's `shape` edges. `VOE_3D_OUTLINE_EDGES` 512 becomes 4096 (a model's
  silhouette); its paragraph says why. The "False … when" list names a model with no loaded
  entry.
- `3d/src/draw_marks.c`: only if the outline call needs the new field passed through.
- `3d/tests/pick.c`, `3d/tests/far.c`: every `voe_3d_pick` call passes NULL for models; a new
  case in `pick.c` hits a model entity at its distance and misses beside it.
- `3d/tests/outline.c`: a case — a model entity outlines to quads; with a NULL store it
  does not. Update the lines in `3d/tests/tests.md` and `3d/src/src.md`.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_3d $(ninja -C build/debug -t
targets all | grep -oE "^voe_test_3d_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R "^3d/"`
exits 0.
