# tests

One plain C program per module, found by the build, checking what the world
promises about ids, rows and queues. None of them needs a graphics card.

- `world.c` — that an id names one thing and then nothing, including the case
  where its slot is live again.
- `component.c` — a row in and out, an iteration that sees every live row once
  across a removal, a full table, an entity's makeup found by walking every
  registered type, that only a runtime-only registration says it is one, and the
  editor's round trip written out once by hand: a row read, copied into a zeroed
  intent value at the offset the type gave, one byte changed, submitted, drained,
  and that byte changed in the table and no other.
- `intent.c` — two submitters in one queue in submission order, and a full queue.
