# src

`app`'s implementation: one file per public header. Every line of reasoning is
in `include/app/`; what is here is the order.

- `app.c` — the two startups, the frame's three readings, the two draw calls,
  and the capture's three steps. Its header says that what it holds is the order.
- `clock.c` — the subtraction and the ceiling.
- `picture.c` — read, decode, upload, each failure handed straight back.
- `pipeline_cache.c` — the path joined, the file read and seeded, the bytes
  written with their two folders made first.
- `start_log.c` — the marks, the sum, and the block put on stderr and appended
  to the file.
- `pace.h` — internal: one step of the pace — draw now, or wait this long —
  focused, out of focus, hidden or closing, as a pure function the caller loops
  over; its header says why (ADR-0215, ADR-0216).
- `pace.c` — the four cases, in the order they are asked.
