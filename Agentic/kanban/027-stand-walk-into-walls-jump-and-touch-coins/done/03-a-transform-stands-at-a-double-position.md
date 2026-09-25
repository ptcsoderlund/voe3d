# 03 — A transform stands at a double position, and its matrix is about an origin
folder: scene
decisions: 0168, 0250

## Change
0250 in `scene`. Rotation and scale stay float. Callers in `3d`, `authoring`, `dev`, `game`,
`editor` and `examples` break and are mended by their own later cards; touch none of them.

- `scene/include/scene/transform_component.h` — `position` becomes `voe_math_double3`, kind
  `DOUBLE3` (`math/double3.h`, card 02). `voe_scene_transform_matrix(transform, voe_math_double3
  origin)`: T·R·S with T the position minus `origin`, subtracted in double and then narrowed.
  Header points: the GPU never sees a world position; every matrix is about a point the caller
  names, normally the eye; at 100 km a float keeps only ~8 mm, a difference of doubles keeps it.
- `scene/src/transform_component.c` — the matrix as above.
- `scene/src/transform_system.c` — the drain's finite check and its report line read the
  position as doubles (the report prints enough digits to tell 100000.25 from 100000.26).
- `scene/include/scene/camera_component.h`, `scene/src/camera_component.c` —
  `voe_scene_camera_view(pose, out)` keeps its signature; it now inverts
  `voe_scene_transform_matrix(pose, pose.position)`, so a view has no translation and sees the
  world relative to its own eye. Header point: a point is taken into the view as its position
  minus the eye, in double, before this matrix.
- `scene/tests/transform.c`, `scene/tests/camera.c` — follow the change; add: a transform at
  (100000.75, 0, 0) about origin (100000.25, 0, 0) translates exactly 0.5; a camera 100 km out
  with a point 1 mm in front of it (offset taken in double) lands 1 mm in front in view space;
  the drain keeps a row whose position is a NaN or infinity.
- `scene/scene.md` — the transform entry says the position is double and the matrix is about an
  origin; the camera entry says the view is relative to its eye.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder scene` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R '^scene/(transform|camera)$'` passes.
