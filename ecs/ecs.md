# ecs

Entities, component tables and intent queues — the shape the world is kept in.
Not any particular component: nothing here knows what a transform or a mesh is,
and the folder that owns a component's meaning is the folder that registers it.

- `include` — the public headers, in `include/ecs/`. See `include/ecs/ecs.md`.
- `src` — the implementation. See `src/src.md`.
- `tests` — one plain C program per module, found by the build. See
  `tests/tests.md`.
