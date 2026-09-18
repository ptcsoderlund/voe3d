# src

`scene`'s implementation, two files per component. The `_component.c` holds the
table's key and the reads anyone may do; the `_system.c` holds the registration,
the one direct creation call and the drain, and every write to that table is in
it. Nothing here names a GPU resource, a file or a graphics API.

- `transform_component.c` — the key, the matrix, and the reads.
- `transform_system.c` — registration, creation, the drain that settles what it
  applies, and the one line a run of settlings writes to stderr.
- `identity_component.c` — the key and the reads, and nothing that writes.
- `identity_system.c` — registration, creation, the drain that settles what it
  applies, and the one line a run of corrections writes to stderr.
- `camera_component.c` — the key, where a camera looks, and the look-at the
  view matrix is. Its header says why the third row is negated.
- `camera_system.c` — registration, and the drain that is the only thing in the
  engine that moves a camera. Holds the speed, the sensitivity and the pitch
  limit, and its header says why each is here rather than at a call site.
- `light_component.c` — the key and the reads, and nothing that writes.
- `light_system.c` — registration, creation, the drain, and the one place a
  light's direction becomes unit length.
