# tests

One plain C program per `scene` module, found by the build, each checking that
module's promises from outside. None of them needs a window or a graphics card.

- `transform.c` — that the matrix is translate·rotate·scale and changes only through a drained
  intent, that a child composes under its parent and back, that a rotation arrives unit length, that
  the field list is the compiler's layout, and that a remembered step blends back by a lag.
- `parent.c` — that a barrel's world place follows its hull, a world with no parent table answers
  the row, the walks go both ways, a loop ends, local then world round-trips, and parenting,
  unparenting and re-parenting keep the world place.
- `identity.c` — that a rename lands only when the system runs, that an
  unterminated name arrives cut and a replaced id arrives put back, and that the
  field list a world hands back marks the id read-only. Its header says why half
  of it submits raw rather than through the typed call.
- `camera.c` — what registration tells a tool, that the view of a pose is its
  inverse with roll kept and none when scaled to nothing, and that a whole-lens
  intent applies a good lens and keeps the row over a bad one.
- `light.c` — what registration tells a tool, that an intent lands only when
  the system runs and a refused one keeps the row, and that a rotation and the
  direction it shines convert both ways, +Z included.
