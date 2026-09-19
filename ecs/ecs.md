# ecs

Entities, component tables and intent queues — the shape the world is kept in.
Not any particular component: nothing here knows what a transform or a mesh is,
and the folder that owns a component's meaning is the folder that registers it.

- `include` — the public headers, in `include/ecs/`; each is listed below by path.
- `src` — the implementation. See `src/src.md`.
- `tests` — one plain C program per module, found by the build. See `tests/tests.md`.
- `include/ecs/world.h` — the world, entity ids and the key a registration is
  made against. Its header says why an id has a generation in it, why the world
  lives in an arena and has no destroy, and why capacities are fixed.
- `include/ecs/component.h` — one table per component type, and the list of
  types a world holds with the key, the description and the replace intent behind
  each; the two markers a registration passes instead of a description,
  runtime-only and compiled out. Its header says how the two directions between
  an entity and its row are built, what a removal does to row order, why iterating
  is handed the arrays rather than an accessor, why a type is described or
  runtime-only and NULL is refused, and why a description is stored and never
  read. On the replace intent — the one a whole row is written through — it
  says why the entity is at offset zero and the row's offset is the declaring folder's to give, why it
  is its own call rather than a parameter to registration, and why a type without
  one is shown and not edited. It also says who writes a row's values and who
  adds and removes rows (0190), and holds each type's default row and the one
  type its rows need (0193), both stored and never read.
- `include/ecs/structure.h` — the world's structural queue: add a row with given
  bytes, remove a row, destroy an entity. Its header says who may submit, why the
  program applies it once a frame before its systems run, and why a request that
  no longer makes sense is dropped silently.
- `include/ecs/intent.h` — a queue per intent type. Its header says why this is
  the only way one module changes another's data, and how little it promises
  about when an intent takes effect.
