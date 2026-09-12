# app

The parts every program's frame loop repeats: a window and a device opened
together, a frame opened, a draw opened and closed. Not the loop itself, and not
what a program decides — system order, a world, arenas, a readout, key bindings
and the present mode are all the program's.

- `include/app/app.h` — the whole public surface: startup, frame open, draw open
  and close, and the window and device to reach past them. Its header shows the
  dozen-line loop a program writes, says why there is no loop, callback or
  function pointer in this folder and what that buys, lists by name what is not
  here and is not coming, and says which of the two arenas is kept and which is
  startup's scratch.
- `include/app/clock.h` — the interval between two frames, as a value. Its
  header says why it takes the reading rather than making one, why `elapsed` and
  `step` are separate numbers and what confusing them costs, and what the first
  tick reports.
- `src/app.c` — the startup pair, the frame's three readings, and the two draw
  calls. Its header says that what it holds is the order.
- `src/clock.c` — the subtraction and the ceiling.
- `tests/clock.c` — the first tick, an ordinary interval, a stall clamped, and
  two ticks at the same reading. Its header says why the clamped case is the one
  that matters. Needs no window and no graphics card.
