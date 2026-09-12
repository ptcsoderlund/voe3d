# ecs

Entities, component tables and intent queues — the shape the world is kept in.
Not any particular component: nothing here knows what a transform or a mesh is,
and the folder that owns a component's meaning is the folder that registers it.

- `include/ecs/world.h` — the world, entity ids and the key a registration is
  made against. Its header says why an id has a generation in it, why the world
  lives in an arena and has no destroy, and why capacities are fixed.
- `include/ecs/component.h` — one table per component type, and the list of
  types a world holds with the key, the description and the replace intent behind
  each. Its header says how the two directions between an entity and its row are
  built, what a removal does to row order, why iterating is handed the arrays
  rather than an accessor, and why a description is stored and never read. On the
  replace intent — the one a whole row is written through — it says why the entity
  is at offset zero and the row's offset is the declaring folder's to give, why it
  is its own call rather than a parameter to registration, and why a type without
  one is shown and not edited.
- `include/ecs/intent.h` — a queue per intent type. Its header says why this is
  the only way one module changes another's data, and how little it promises
  about when an intent takes effect.
- `src/world_internal.h` — the struct the three files below share, and where the
  split between them runs.
- `src/world.c` — the entity slots: what a create takes, what a destroy gives
  back, and why a create prefers a slot something used to be in.
- `src/component.c` — registration, the type list, the row lookups, and the swap
  a removal does. Its header says which three things a removal has to keep in
  step.
- `src/intent.c` — the queues, and why a drain is all or nothing.
- `tests/world.c` — that an id names one thing and then nothing, including the
  case where its slot is live again.
- `tests/component.c` — a row in and out, an iteration that sees every live row
  once across a removal, a full table, an entity's makeup found by walking every
  registered type, and the editor's round trip written out once by hand: a row
  read, copied into a zeroed intent value at the offset the type gave, one byte
  changed, submitted, drained, and that byte changed in the table and no other.
- `tests/intent.c` — two submitters in one queue in submission order, and a full
  queue.
