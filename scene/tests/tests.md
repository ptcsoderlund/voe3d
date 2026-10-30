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
- `identity.c` — that a name, id, fold and switch change only through a drained intent, the
  drain cuts an unterminated name and puts back a replaced id, the id is read-only, and a switch
  reaches the children and reads on without the tables.
- `camera.c` — what registration tells a tool, that the view of a pose is its
  inverse with roll kept and none when scaled to nothing, and that a whole-lens
  intent applies a good lens and keeps the row over a bad one.
- `light.c` — the sun's registration, intents, bounces, bounce strength, cast
  shadows, disc and glow sizes and their ranges (disc 0 to 10 °, glow 0 to 1),
  and the conversions between a rotation and the direction it shines.
- `point_light.c` — the point light's registration, refused and accepted
  replaces, falloff bounds and its range (0.25 to 4), cast shadows, bounces and
  bounce strength.
- `light_blocker.c` — the blocker's registration with a Block All default, the
  names All, Fill, Direct and the former name "kind", a row read back, a
  replace to Direct landing on a run, refused negative, infinite and NaN sizes
  and a Block past Direct, and a dead entity.
- `sky.c` — the sky's registration, its default row of strength 1 and five
  fields, horizon height's range 0 to 1, a row read back, a horizon replace landing on a run, a refused NaN
  zenith, negative ground, horizon height 1.5, strength -1 and infinite, and a
  dead entity.
- `haze.c` — the haze's registration, its default row of haze 1000, start 100
  and height 1000 and three fields, a row read back, a haze replace landing on
  a run, height -50 taken, a refused NaN haze, haze -1, start -1, infinite
  start, NaN height and infinite height, and a dead entity.
- `clouds.c` — the clouds' registration, fields, kind names and ranges
  (coverage, shadow strength 0 to 1, wind direction 0 to 360 °), the drift
  runtime-only, a row read back, a replace landing on a run, each refusal
  keeping the row, the drift moved by the wind and dropped with its row.
- `fog_volumes.c` — the fog volume's registration, fields and ranges
  (thickness, patchiness, godrays strength 0 to 1, wind direction 0 to 360 °),
  a row read back, a replace landing on a run, each refusal keeping the row,
  the drift moved by the wind and dropped with its row.
- `picture.c` — the picture's registration, its exposure 0 and Soft default,
  the tone names Clip and Soft, exposure's range -16 to 16 stops, a row read back, an exposure replace landing on
  a run, a refused NaN exposure, exposure 17 and tone 2, and a dead entity.
