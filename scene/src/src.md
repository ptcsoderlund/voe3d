# src

`scene`'s implementation, two files per component. The `_component.c` holds the
table's key and the reads anyone may do; the `_system.c` holds the registration,
the one direct creation call and the drain, and every write to that table is in
it. Nothing here names a GPU resource, a file or a graphics API.

- `transform_component.c` — the key, the matrix, and the reads.
- `transform_system.c` — registration, creation, the drain that settles what it
  applies, and the one line a run of settlings writes to stderr.
- `transform_previous.c` — the runtime-only previous table, the copy a step
  starts with, and the world place a lag back, blended link by link.
- `parent_component.c` — the key, the read, and the walks up and down the tree.
- `parent_system.c` — registration, and parenting as structural requests plus
  one transform intent; nothing that drains.
- `prefab_component.c` — the two keys and the two reads.
- `prefab_system.c` — registration of both tables; nothing that drains.
- `identity_component.c` — the key and the reads, and nothing that writes.
- `identity_system.c` — registration, creation, the drain that settles what it
  applies, and the one line a run of corrections writes to stderr.
- `camera_component.c` — the key, the reads, and the view as the inverse of a
  pose's matrix, false when it has none.
- `camera_system.c` — registration, creation, and the drain that applies a
  whole lens or keeps the row over one that cannot project.
- `light_component.c` — the key, the reads, and the conversions between a
  rotation and the direction it shines.
- `light_system.c` — registration, creation, and the drain that applies a whole
  light or keeps the row over one it refuses.
- `point_light_component.c` — the two keys, the reads, and the strength a reader
  draws with.
- `point_light_system.c` — registration, creation, and the run that applies whole
  lights and flashes, keeps one glow per light and fades them.
