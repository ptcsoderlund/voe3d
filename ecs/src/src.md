# src

`ecs`'s implementation, split across the world, its component tables, its
intent queues and its structural queue.

- `world_internal.h` — the struct the four files below share, and where the
  split between them runs.
- `world.c` — the entity slots: what a create takes, what a destroy gives
  back, and why a create prefers a slot something used to be in.
- `component.c` — registration, the type list, the row lookups, and the swap
  a removal does. Its header says which three things a removal has to keep in
  step.
- `intent.c` — the queues, and why a drain is all or nothing.
- `structure.c` — the structural queue, and why applying it goes through the
  same calls anyone else would make.
