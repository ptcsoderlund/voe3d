# scene

Where things are and what looks at them: the components a person would author,
and the systems that own them. Nothing here names a GPU resource, a file or a
graphics API — a projection matrix is `3d`'s, because clip space is.

- `include/scene/transform_component.h` — position, rotation, scale, and the
  matrix they become. Its header says why there is no parent, why the matrix is
  not stored, and in which order the three are composed.
- `include/scene/transform_system.h` — the intent that moves one, and the direct
  call that creates one. Its header says why the intent carries the whole
  transform and why creation is not one.
- `include/scene/camera_component.h` — eye, yaw, pitch, field of view and the two
  planes, and the view matrix. Its header says why the projection is not built
  here and why orientation is two angles.
- `include/scene/camera_system.h` — the two camera intents, absolute and
  relative. Its header says why there are two, in which order they apply, and
  which of the numbers a caller passes are the mouse's own.
- `src/transform_component.c` — the key, the matrix, and the reads.
- `src/transform_system.c` — registration, creation, and the drain.
- `src/camera_component.c` — the key, where a camera looks, and the look-at the
  view matrix is. Its header says why the third row is negated.
- `src/camera_system.c` — registration, and the drain that is the only thing in
  the engine that moves a camera. Holds the speed, the sensitivity and the pitch
  limit, and its header says why each is here rather than at a call site.
- `tests/transform.c` — that the matrix is translate·rotate·scale and that an
  intent lands only when the system runs.
- `tests/camera.c` — the view matrix, and every way of flying one that draws a
  plausible picture while being wrong. Its header says where these came from.
