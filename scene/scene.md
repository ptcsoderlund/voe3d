# scene

Where things are, what looks at them and what lights them: the components a
person would author, and the systems that own them. Nothing here names a GPU
resource, a file or a graphics API — a projection matrix is `3d`'s, because clip
space is.

- `include` — the public headers, in `include/scene/`; each is listed below by path.
- `src` — the implementation. See `src/src.md`.
- `tests` — one plain C program per module, found by the build. See `tests/tests.md`.
- `include/scene/transform_component.h` — a double position, a rotation and a
  scale relative to a parent, the matrix they become about an origin the caller
  names, and the world place composed up the chain.
- `include/scene/parent_component.h` — the entity a thing hangs under, and the
  walks up to an ancestor and down a tree, each capped at 32 links.
- `include/scene/parent_system.h` — registering the parent table (no replace, no
  menu path), and the one call that parents or unparents keeping the world place.
- `include/scene/prefab_component.h` — the prefab file a placed copy's root was
  placed from, saved, and the runtime-only row naming the copy each part was
  made for. Its header says what a copy saves and why parts carry identities.
- `include/scene/prefab_system.h` — registering both tables: no replace, no menu
  path, the part table runtime-only.
- `include/scene/transform_system.h` — the intent that moves one, registered as
  the component's replace, the call that creates one, and the opt-in previous
  table a stepping world remembers each step and blends back to by a lag
  (each link of the chain, composed into a world transform).
- `include/scene/camera_component.h` — the lens only: field of view and the two
  planes, as a described field list, and the view from a pose, relative to its
  own eye. Its header says why the pose is the transform, why the view is the
  plain inverse, and why the projection is not built here.
- `include/scene/camera_system.h` — the whole-lens intent, registered as the
  camera's replace, and the transform a camera needs. Its header says why the
  intent is whole, what lens is refused, and why Add component never offers one.
- `include/scene/identity_component.h` — a 64-bit id, a name and whether its row is folded on
  the entities a person authored, the id read-only. Its header says why its
  presence is what "authored" means, why the id is unique within a file, and
  what its drain corrects.
- `include/scene/identity_system.h` — the intent that renames one, and the
  direct call that creates one. Its header says why creation is not an intent,
  which of the two checks is an assert and which a correction, and why the
  report's state is per process.
- `include/scene/light_component.h` — the directional light: its linear colour
  and strength, a fill of the shade and a named count of bounces, as a described
  field list, and the conversions between a rotation and its direction. Its
  header says why that is the transform's -Z.
- `include/scene/light_system.h` — the whole-light intent, registered as the
  light's replace, the direct call that creates one, and the transform it needs.
  Its header says what light is refused and why a drawn world needs the table.
- `include/scene/point_light_component.h` — a lamp beside the sun: linear colour,
  intensity, range and a flash, as a described field list, the runtime-only glow
  row and the strength a reader draws with. Its header says where it shines from
  and what starts a flash.
- `include/scene/point_light_system.h` — the whole-light intent as its replace,
  the flash intent, the direct call that creates one, and the run that drains
  both and fades the glows. Its header says what is refused and why both queue.
