# src

`scene`'s implementation, two files per component. The `_component.c` holds the
table's key and the reads anyone may do; the `_system.c` holds the registration,
the one direct creation call and the drain, and every write to that table is in
it. Nothing here names a GPU resource, a file or a graphics API.

- `transform_component.c` — the key, the matrix, and the reads.
- `transform_system.c` — registration, creation, the drain that settles what it
  applies, and the one line a run of settlings writes to stderr.
- `transform_previous.c` — the runtime-only previous table, the copy a step
  starts with, and the blend a lag back from the current transform.
- `identity_component.c` — the key and the reads, and nothing that writes.
- `identity_system.c` — registration, creation, the drain that settles what it
  applies, and the one line a run of corrections writes to stderr.
- `camera_component.c` — the key, the reads, and the view as the inverse of a
  pose's matrix, false when it has none.
- `camera_system.c` — registration, creation, and the drain that applies a
  whole lens or keeps the row over one that cannot project.
- `light_component.c` — the key and the reads, and nothing that writes.
- `light_system.c` — registration, creation, the drain, and the one place a
  light's direction becomes unit length.
