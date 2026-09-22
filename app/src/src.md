# src

`app`'s implementation: one file per public header. Every line of reasoning is
in `include/app/`; what is here is the order.

- `app.c` — the two startups, the frame's three readings, the two draw calls,
  and the capture's three steps. Its header says that what it holds is the order.
- `clock.c` — the subtraction and the ceiling.
- `pace.h` — internal: how long a frame waits, focused, out of focus or hidden,
  as a pure function; its header says why (ADR-0215).
- `pace.c` — the three cases.
