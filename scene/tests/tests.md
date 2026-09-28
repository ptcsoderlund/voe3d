# tests

One plain C program per `scene` module, found by the build, each checking that
module's promises from outside. None of them needs a window or a graphics card.

- `transform.c` — that the matrix is translate·rotate·scale, a child composes under a parent and
  relative undoes it (a far millimetre kept, a zero scale finite), an intent lands when the system runs,
  a rotation arrives unit length (a zero one leaves the row alone), the field list is the
  compiler's layout, and a remembered step blends back by a lag.
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
