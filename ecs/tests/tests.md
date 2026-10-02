# tests

One plain C program per `ecs` module, found by the build, each checking that
module's promises from outside.

- `world.c` — that an id names one thing and then nothing, including the
  case where its slot is live again.
- `component.c` — a row in and out, iteration seeing each live row once across a removal, a
  full table, an entity's makeup from every registered type, runtime-only registration, the editor's
  typed intent round trip, a default and an unsaid row read back set and unset, and a menu path.
- `intent.c` — two submitters in one queue in submission order, and a full
  queue.
- `structure.c` — structural requests landing only at apply, in submission
  order, every drop case dropped, a queue full of requests or of bytes, and a
  world without a queue refusing every submit.
