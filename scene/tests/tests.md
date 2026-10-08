# tests

One plain C program per `scene` module, found by the build, each checking that
module's promises from outside. None of them needs a window or a graphics card.

- `transform.c` — that the matrix is translate·rotate·scale and changes only through a drained
  intent, a child composes under its parent, a rotation arrives unit length, and a remembered step
  blends back by a lag.
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
- `light.c` — the sun's registration, intents, bounces, bounce strength, cast
  shadows, and the conversions between a rotation and the direction it shines.
- `point_light.c` — the point light's registration, refused and accepted
  replaces, falloff bounds, cast shadows, bounces and bounce strength.
- `light_blocker.c` — the blocker's registration with a Block All default, the
  names All, Fill, Direct and the former name "kind", a row read back, a
  replace to Direct landing on a run, refused negative, infinite and NaN sizes
  and a Block past Direct, and a dead entity.
