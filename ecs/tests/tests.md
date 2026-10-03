# tests

One plain C program per `ecs` module, found by the build, each checking that
module's promises from outside.

- `world.c` — that an id names one thing and then nothing, including the
  case where its slot is live again.
- `component.c` — rows in and out, iteration across a removal, a full table and
  a type's capacity, an entity's makeup, registration markers, the typed intent
  round trip, defaults and unsaid rows, and a menu path.
- `intent.c` — two submitters in one queue in submission order, and a full
  queue.
- `structure.c` — structural requests landing only at apply, in submission
  order, every drop case dropped, a queue full of requests or of bytes, and a
  world without a queue refusing every submit.
