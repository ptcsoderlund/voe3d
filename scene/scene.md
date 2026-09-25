# scene

Where things are, what looks at them and what lights them: the components a
person would author, and the systems that own them. Nothing here names a GPU
resource, a file or a graphics API — a projection matrix is `3d`'s, because clip
space is.

- `include` — the public headers, in `include/scene/`; each is listed below by path.
- `src` — the implementation. See `src/src.md`.
- `tests` — one plain C program per module, found by the build. See `tests/tests.md`.
- `include/scene/transform_component.h` — a double position, a rotation and a
  scale as a described field list, and the matrix they become about an origin
  the caller names. The file says why there is no parent, why the matrix is not
  stored, and what the drain settles.
- `include/scene/transform_system.h` — the intent that moves one, registered as
  the component's replace, the call that creates one, and the opt-in previous
  table a stepping world remembers each step and blends back to by a lag
  (position in double, rotation the shorter way, scale straight).
- `include/scene/camera_component.h` — the lens only: field of view and the two
  planes, as a described field list, and the view from a pose, relative to its
  own eye. Its header says why the pose is the transform, why the view is the
  plain inverse, and why the projection is not built here.
- `include/scene/camera_system.h` — the whole-lens intent, registered as the
  camera's replace, and the transform a camera needs. Its header says why the
  intent is whole, what lens is refused, and why Add component never offers one.
- `include/scene/identity_component.h` — a 64-bit id and a 64-byte name on the
  entities a person authored, the id read-only. Its header says why the
  component's presence is the whole of what "authored" means, why the id is
  unique within a file, not a world, and what its drain corrects.
- `include/scene/identity_system.h` — the intent that renames one, and the
  direct call that creates one. Its header says why creation is not an intent,
  which of the two checks is an assert and which a correction, and why the
  report's state is per process.
- `include/scene/light_component.h` — the sun: which way its light travels, its
  colour and its strength, written as a described field list. Its header says why a direction is where the light
  goes rather than where the sun is, and why a reader never has to normalize it.
- `include/scene/light_system.h` — the intent that turns it, and the direct call
  that creates one. Its header says where the normalization happens and why a
  world that is drawn needs this table even when it holds no sun, and where it
  sits in Add component.
