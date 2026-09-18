# tests

`app`'s own tests: plain C programs with an ordinary `main()`, zero for pass,
found by the build and registered nowhere.

- `capture.c` — a headless app draws one asymmetrical frame, writes it to a PNG
  and reads the file back to the colours it drew; a path that cannot be opened
  is refused and writes nothing. Its header says why the picture is
  asymmetrical. Needs a graphics card, and skips with a reason without one.
- `clock.c` — the first tick, an ordinary interval, a stall clamped, and two
  ticks at the same reading. Its header says why the clamped case is the one
  that matters. Needs no window and no graphics card.
