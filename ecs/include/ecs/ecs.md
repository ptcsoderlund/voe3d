# ecs

`ecs`'s public headers: the world, the component tables and the intent queues.
Nothing here knows what a transform or a mesh is.

- `world.h` — the world, entity ids and the key a registration is made against.
  Its header says why an id has a generation in it, why the world lives in an
  arena and has no destroy, and why capacities are fixed.
- `component.h` — one table per component type, and the list of types a world
  holds with the key, the description and the replace intent behind each; the two
  markers a registration passes instead of a description, runtime-only and
  compiled out. Its header says how the two directions between an entity and its
  row are built, what a removal does to row order, why iterating is handed the
  arrays rather than an accessor, why a type is described or runtime-only and
  NULL is refused, and why a description is stored and never read. On the replace
  intent — the one a whole row is written through — it says why the entity is at
  offset zero and the row's offset is the declaring folder's to give, why it is
  its own call rather than a parameter to registration, and why a type without
  one is shown and not edited.
- `intent.h` — a queue per intent type. Its header says why this is the only way
  one module changes another's data, and how little it promises about when an
  intent takes effect.
