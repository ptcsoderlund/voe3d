# src

`physics`'s implementation. The `_component.c` holds the table's key and the
reads; the `_system.c` holds the registration, creation and the drain, and
every write to that table is in it.

- `collider_component.c` — the key, the kind names and the reads.
- `collider_system.c` — registration, creation, the drain that settles what it
  applies, and the report a run of settlings writes.
- `overlap.c` — the query as a segment and radius against each collider's
  sphere, capsule or box.
- `shape.c` — a collider and its transform turned into a shape in the world.
