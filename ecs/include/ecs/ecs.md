# ecs

The public headers, one entry each; the fuller account of every one of these
stays on `ecs/ecs.md`.

- `world.h` — the world, entity ids and the key a registration is made against.
- `component.h` — one table per component type, and the two directions between
  an entity and its row.
- `intent.h` — a queue per intent type, and the only way one module changes
  another's data.
- `structure.h` — the world's structural queue, through which rows are added and
  removed and entities destroyed.
