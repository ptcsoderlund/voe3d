# scene

`scene`'s public headers: the components a person would author — transform,
camera, identity and the sun — each beside the system that owns it. Nothing here
names a GPU resource, a file or a graphics API.

- `transform_component.h` — position, rotation, scale, written as a described
  field list, and the matrix they become. Its header says why there is no parent,
  why the matrix is not stored, in which order the three are composed, and what
  the drain settles before it writes one — the two bounds a rotation is held to
  are named here.
- `transform_system.h` — the intent that moves one, registered as the component's
  replace, and the direct call that creates one. Its header says why the intent
  carries the whole transform, why creation is not one, why neither call checks a
  rotation and the drain checks every one, and why the report's state is per
  process.
- `camera_component.h` — eye, yaw, pitch, field of view and the two planes,
  written as a described field list, and the view matrix. Its header says why the
  projection is not built here and why orientation is two angles.
- `camera_system.h` — the two camera intents, absolute and relative. Its header
  says why there are two, in which order they apply, and which of the numbers a
  caller passes are the mouse's own.
- `identity_component.h` — a 64-bit id and a 64-byte name on the entities a
  person authored, the id marked read-only. Its header says why the component's
  presence is the whole of what "authored" means, why the id is unique within a
  file and not within a world, and what its drain corrects.
- `identity_system.h` — the intent that renames one, and the direct call that
  creates one. Its header says why creation is not an intent, which of the two
  checks is an assert and which a correction, and why the report's state is per
  process.
- `light_component.h` — the sun: which way its light travels, its colour and its
  strength, written as a described field list. Its header says why a direction is
  where the light goes rather than where the sun is, and why a reader never has
  to normalize it.
- `light_system.h` — the intent that turns it, and the direct call that creates
  one. Its header says where the normalization happens and why a world that is
  drawn needs this table even when it holds no sun.
