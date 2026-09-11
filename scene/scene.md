# scene

Where things are, what looks at them and what lights them: the components a
person would author, and the systems that own them. Nothing here names a GPU
resource, a file or a graphics API — a projection matrix is `3d`'s, because clip
space is.

- `include/scene/transform_component.h` — position, rotation, scale, written as a
  described field list, and the matrix they become. Its header says why there is
  no parent, why the matrix is not stored, and in which order the three are
  composed.
- `include/scene/transform_system.h` — the intent that moves one, and the direct
  call that creates one. Its header says why the intent carries the whole
  transform and why creation is not one.
- `include/scene/camera_component.h` — eye, yaw, pitch, field of view and the two
  planes, and the view matrix. Its header says why the projection is not built
  here and why orientation is two angles.
- `include/scene/camera_system.h` — the two camera intents, absolute and
  relative. Its header says why there are two, in which order they apply, and
  which of the numbers a caller passes are the mouse's own.
- `include/scene/light_component.h` — the sun: which way its light travels, its
  colour and its strength. Its header says why a direction is where the light
  goes rather than where the sun is, and why a reader never has to normalize it.
- `include/scene/light_system.h` — the intent that turns it, and the direct call
  that creates one. Its header says where the normalization happens and why a
  world that is drawn needs this table even when it holds no sun.
- `src/transform_component.c` — the key, the matrix, and the reads.
- `src/transform_system.c` — registration, creation, and the drain.
- `src/camera_component.c` — the key, where a camera looks, and the look-at the
  view matrix is. Its header says why the third row is negated.
- `src/camera_system.c` — registration, and the drain that is the only thing in
  the engine that moves a camera. Holds the speed, the sensitivity and the pitch
  limit, and its header says why each is here rather than at a call site.
- `src/light_component.c` — the key and the reads, and nothing that writes.
- `src/light_system.c` — registration, creation, the drain, and the one place a
  light's direction becomes unit length.
- `tests/transform.c` — that the matrix is translate·rotate·scale and that an
  intent lands only when the system runs.
- `tests/camera.c` — the view matrix, and every way of flying one that draws a
  plausible picture while being wrong. Its header says where these came from.
- `tests/light.c` — that a direction arrives unit length whichever of the two
  writes it came through, and that an intent lands only when the system runs.
  Its header says why the normalization is the claim worth a test.
