# tests

One plain C program per `scene` module, found by the build, each checking that
module's promises from outside. None of them needs a window or a graphics card.

- `transform.c` — that the matrix is translate·rotate·scale, that an intent
  lands only when the system runs, that a rotation arrives unit length and one
  with no length leaves the row alone, and that the field list a world hands
  back for a transform is the one the compiler laid out. Its header says why
  half of it submits raw rather than through the typed call.
- `identity.c` — that a rename lands only when the system runs, that an
  unterminated name arrives cut and a replaced id arrives put back, and that the
  field list a world hands back marks the id read-only. Its header says why half
  of it submits raw rather than through the typed call.
- `camera.c` — the view matrix, every way of flying one that draws a plausible
  picture while being wrong, and that the field list a world hands back is the
  one the compiler laid out. Its header says where these came from.
- `light.c` — that a direction arrives unit length whichever of the two writes
  it came through, that an intent lands only when the system runs, and that the
  field list a world hands back is the one the compiler laid out. Its header
  says why the normalization is the claim worth a test.
