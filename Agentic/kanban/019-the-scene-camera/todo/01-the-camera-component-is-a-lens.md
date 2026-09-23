# 01 — The camera component is a lens on a transformed entity
folder: scene
decisions: 0168, 0218, 0222, 0223

## Change
The camera holds only its lens; where it is and how it faces is its entity's transform.

- `scene/include/scene/camera_component.h` — fields become `fov_y`, `near_plane`, `far_plane` only (eye, yaw,
  pitch go). `voe_scene_camera_forward` goes. `voe_scene_camera_view` becomes
  `[[nodiscard]] bool voe_scene_camera_view(voe_scene_transform pose, voe_math_float4x4 *out)`: the inverse of
  `voe_scene_transform_matrix(pose)`, scale and roll included; false with `out` untouched when the matrix has no
  inverse, by the singularity test `math/include/math/float4x4.h`'s `_inverse` asserts on (use
  `voe_math_float4x4_determinant`). Header points: the component is the lens only and the pose is the transform
  (0222); why the view is the plain inverse and nothing is stripped; that a pose with no inverse sees nothing
  (0223); the projection paragraph stays; the "two angles" and "no replace intent" paragraphs go.
- `scene/src/camera_component.c` — follows.
- `scene/include/scene/camera_system.h` — the placement, the motion, their two submit calls and their header
  paragraphs go. New: `voe_scene_camera_intent { voe_ecs_entity entity; voe_scene_camera camera; }` and
  `[[nodiscard]] bool voe_scene_camera_submit(voe_ecs_world *, voe_scene_camera_intent)` (false when the queue
  is full). `voe_scene_camera_register` registers the table, description, one intent queue, the intent as the
  replace (`voe_ecs_component_replace_set`), the default row (60°, 0.1, 1000) and the transform as the needed
  type (`voe_ecs_component_needs_set`); the transform must be registered first (assert). No menu path, as today.
  `voe_scene_camera_system_run` drains the queue: a dead entity or one with no camera is dropped silently; a
  lens refused by 0223's rule keeps the row and writes one `error:` line on stderr naming the entity and field;
  a good one replaces the row. Header points: the whole-lens intent and why whole (as the transform's); the
  refusal rule; the camera needs a transform; moved only by transform intents (0222). Read
  `scene/include/scene/transform_system.h` for the shape to mirror.
- `scene/src/camera_system.c` — follows; the speed, sensitivity and pitch constants go with the motion.
- `scene/tests/camera.c` — rewritten for the new surface: registration sets the replace, the default row, the
  needed type and no menu path; the view of a pose at (0, 0, 5) unrotated takes (0, 0, 0) to (0, 0, −5); a pose at
  the origin rolled +90° about Z takes (0, 1, −1) to (1, 0, −1); a pose with a
  zero scale component returns false; a good lens intent applies; a fov of 0, of π, a near of 0, a far at near,
  and a NaN each leave the row; an intent on a dead entity is dropped.
- `scene/scene.md` and `scene/tests/tests.md`, `scene/src/src.md` — the camera entries say lens, the view from
  a pose, the whole-lens intent.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `scene` exits 0, and
`grep -rnE "camera_place|camera_move|camera_forward|\.yaw|\.pitch|\.eye" scene` prints nothing.
