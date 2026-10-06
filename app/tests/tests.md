# tests

`app`'s own tests: plain C programs with an ordinary `main()`, zero for pass,
found by the build and registered nowhere.

- `capture.c` — a headless app draws one asymmetrical frame, writes it to a
  PNG and reads the file back to the colours it drew; a path that cannot be
  opened is refused and writes nothing. Needs a graphics card, and skips with a
  reason without one.
- `pace.c` — focused draws, hidden waits with no timeout, out of focus waits the
  rest of the heartbeat, and closing draws from every state; plus the storms,
  which run app.c's loop against a wait that returns after a millisecond however
  long it asked for. Needs no window and no graphics card.
- `clock.c` — the first tick, an ordinary interval, a stall clamped, and two
  ticks at the same reading. Its header says why the clamped case is the one
  that matters. Needs no window and no graphics card.
- `picture.c` — a 4x2 PNG written to the temp folder comes back as a texture of
  that size; a missing file and a file of text are refused. Needs a graphics
  card, and skips with a reason without one.
- `pipeline_cache.c` — a headless device saves its cache where no folder was
  yet, a second loads it and prepares, a garbage file and a missing path cost
  nothing, and the path ends in `voe3d/<name>`. Skips without a graphics card.
- `start_log.c` — steps come back in order after "before main" and add up, a
  file keeps two starts, and a file over 64 KiB holds only the new block. Needs
  no window and no graphics card.
