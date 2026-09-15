# app

The parts every program's frame loop repeats: a window and a device opened
together — or a device alone, with no window at all — a frame opened, a draw
opened and closed, and what was drawn written out as a picture. Not the loop
itself, and not what a program decides — system order, a world, arenas, a
readout, key bindings and the present mode are all the program's.

- `include/app/app.h` — the whole public surface: the two startups, frame open,
  draw open and close, a capture, and the window and device to reach past them.
  Its header shows the dozen-line loop a program writes, says why there is no
  loop, callback or function pointer in this folder and what that buys, lists by
  name what is not here and is not coming, says which of the two arenas is kept
  and which is startup's scratch, and says what a frame reports and what the
  window is when a program opened with no display.
- `include/app/clock.h` — the interval between two frames, as a value. Its
  header says why it takes the reading rather than making one, why `elapsed` and
  `step` are separate numbers and what confusing them costs, and what the first
  tick reports.
- `src/app.c` — the two startups, the frame's three readings, the two draw calls,
  and the capture's three steps. Its header says that what it holds is the order.
- `src/clock.c` — the subtraction and the ceiling.
- `tests/capture.c` — a headless app draws one asymmetrical frame, writes it to a
  PNG and reads the file back to the colours it drew; a path that cannot be
  opened is refused and writes nothing. Its header says why the picture is
  asymmetrical. Needs a graphics card, and skips with a reason without one.
- `tests/clock.c` — the first tick, an ordinary interval, a stall clamped, and
  two ticks at the same reading. Its header says why the clamped case is the one
  that matters. Needs no window and no graphics card.
