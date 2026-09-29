# src

`physics`'s implementation. The `_component.c` holds the table's key and the
reads; the `_system.c` holds the registration, creation and the drain, and
every write to that table is in it, but for the body's move.

- `body_component.c` — the key and the reads.
- `body_move.c` — the move: substeps, push-out, the step up and staying on the
  floor, out as transform intents and the body's own rows.
- `body_system.c` — registration, creation, the drain that keeps a bad row, and
  the report a run of refusals writes.
- `collider_component.c` — the key, the kind names and the reads.
- `collider_system.c` — registration, creation, the drain that settles what it
  applies, and the report a run of settlings writes.
- `overlap.c` — the query as a segment and radius against each collider's
  sphere, capsule or box.
- `shape.c` — a collider and its transform turned into a shape in the world.
- `sweep.c` — the gather, and the sweep as a segment and radius against each
  obstacle's sphere or capsule; boxes passed over.
