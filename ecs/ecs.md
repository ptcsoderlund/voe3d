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
- `include/ecs/component.h` — one table per component type, and the types a
  world holds with the key, capacity, description, replace intent, default and
  unsaid rows, former field names, needed type and menu path; the two markers a
  registration passes instead of a description.
- `include/ecs/structure.h` — the world's structural queue: add a row with given
  bytes, remove a row, destroy an entity. Its header says who may submit, why the
  program applies it once a frame before its systems run, and why a request that
  no longer makes sense is dropped silently.
- `include/ecs/intent.h` — a queue per intent type. Its header says why this is
  the only way one module changes another's data, and how little it promises
  about when an intent takes effect.
