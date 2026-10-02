# tests

One plain C program per `scene` module, found by the build, each checking that
module's promises from outside. None of them needs a window or a graphics card.

- `transform.c` — that the matrix is translate·rotate·scale and changes only through a drained
  intent, that a child composes under its parent and back, that a rotation arrives unit length, that
  the field list is the compiler's layout, and that a remembered step blends back by a lag.
- `parent.c` — that a child's world place follows its parent, the walks go both ways and end on a
  loop, reparenting keeps the world place, and entities without a transform parent and end chains.
- `prefab.c` — that both prefab tables register, queued rows read back once
  applied, the part table is runtime-only and the prefab table is not, and it
  has no replace and no menu.
- `identity.c` — that a name, id and fold change only through a drained intent, the drain cuts an
  unterminated name and puts back a replaced id, and the field list marks the id read-only.
- `camera.c` — what registration tells a tool, that the view of a pose is its
  inverse with roll kept and none when scaled to nothing, and that a whole-lens
  intent applies a good lens and keeps the row over a bad one.
- `light.c` — what registration tells a tool, that an intent lands only when
  the system runs and a refused one keeps the row, that bounces lands up to its
  maximum and is named "0" and "1", and that a rotation and the direction it
  shines convert both ways, +Z included.
- `point_light.c` — what registration tells a tool (falloff 1 by default), that
  a refused replace keeps the row, a falloff past either bound included, that an
  accepted one changes falloff and intensity after one run, and that a falloff
  of exactly either bound is accepted.
