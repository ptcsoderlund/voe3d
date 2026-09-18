# scene

Where things are, what looks at them and what lights them: the components a
person would author, and the systems that own them. Nothing here names a GPU
resource, a file or a graphics API — a projection matrix is `3d`'s, because clip
space is.

- `include` — the public headers, in `include/scene/`; each is listed below by path.
- `src` — the implementation; each file is listed below by path.
- `tests` — one plain C program per module, found by the build; each is listed below by path.
- `include/scene/transform_component.h` — position, rotation, scale, written as a
  described field list, and the matrix they become. Its header says why there is
  no parent, why the matrix is not stored, in which order the three are composed,
  and what the drain settles before it writes one — the two bounds a rotation is
  held to are named here.
- `include/scene/transform_system.h` — the intent that moves one, registered as
  the component's replace, and the direct call that creates one. Its header says
  why the intent carries the whole transform, why creation is not one, why
  neither call checks a rotation and the drain checks every one, and why the
  report's state is per process.
- `include/scene/camera_component.h` — eye, yaw, pitch, field of view and the two
  planes, written as a described field list, and the view matrix. Its header says why the projection is not built
  here and why orientation is two angles.
- `include/scene/camera_system.h` — the two camera intents, absolute and
  relative. Its header says why there are two, in which order they apply, and
  which of the numbers a caller passes are the mouse's own.
- `include/scene/identity_component.h` — a 64-bit id and a 64-byte name on the
  entities a person authored, the id marked read-only. Its header says why the
  component's presence is the whole of what "authored" means, why the id is
  unique within a file and not within a world, and what its drain corrects.
- `include/scene/identity_system.h` — the intent that renames one, and the
  direct call that creates one. Its header says why creation is not an intent,
  which of the two checks is an assert and which a correction, and why the
  report's state is per process.
- `include/scene/light_component.h` — the sun: which way its light travels, its
  colour and its strength, written as a described field list. Its header says why a direction is where the light
  goes rather than where the sun is, and why a reader never has to normalize it.
- `include/scene/light_system.h` — the intent that turns it, and the direct call
  that creates one. Its header says where the normalization happens and why a
  world that is drawn needs this table even when it holds no sun.
- `src/transform_component.c` — the key, the matrix, and the reads.
- `src/transform_system.c` — registration, creation, the drain that settles what
  it applies, and the one line a run of settlings writes to stderr.
- `src/identity_component.c` — the key and the reads, and nothing that writes.
- `src/identity_system.c` — registration, creation, the drain that settles what
  it applies, and the one line a run of corrections writes to stderr.
- `src/camera_component.c` — the key, where a camera looks, and the look-at the
  view matrix is. Its header says why the third row is negated.
- `src/camera_system.c` — registration, and the drain that is the only thing in
  the engine that moves a camera. Holds the speed, the sensitivity and the pitch
  limit, and its header says why each is here rather than at a call site.
- `src/light_component.c` — the key and the reads, and nothing that writes.
- `src/light_system.c` — registration, creation, the drain, and the one place a
  light's direction becomes unit length.
- `tests/transform.c` — that the matrix is translate·rotate·scale, that an
  intent lands only when the system runs, that a rotation arrives unit length and
  one with no length leaves the row alone, and that the field list a world hands
  back for a transform is the one the compiler laid out. Its header says why half
  of it submits raw rather than through the typed call.
- `tests/identity.c` — that a rename lands only when the system runs, that an
  unterminated name arrives cut and a replaced id arrives put back, and that the
  field list a world hands back marks the id read-only. Its header says why half
  of it submits raw rather than through the typed call.
- `tests/camera.c` — the view matrix, every way of flying one that draws a
  plausible picture while being wrong, and that the field list a world hands back
  is the one the compiler laid out. Its header says where these came from.
- `tests/light.c` — that a direction arrives unit length whichever of the two
  writes it came through, that an intent lands only when the system runs, and
  that the field list a world hands back is the one the compiler laid out.
  Its header says why the normalization is the claim worth a test.
