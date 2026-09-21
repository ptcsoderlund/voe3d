# app

The parts every program's frame loop repeats: a window and a device opened
together — or a device alone, with no window at all — a frame opened, a draw
opened and closed, and what was drawn written out as a picture. Not the loop
itself, and not what a program decides — system order, a world, arenas, a
readout, key bindings and the present mode are all the program's.

- `include` — the public headers, in `include/app/`; each is listed below by path.
- `src` — the implementation: the startups, the frame and draw calls, the
  capture, and the clock's arithmetic; each file is listed on `src/src.md`.
- `tests` — one plain C program per module, found by the build; each is listed on
  `tests/tests.md`.
- `include/app/app.h` — the whole public surface: the two startups, frame open,
  draw open and close, a capture, and the window and device to reach past them.
  Its header shows the loop a program writes and why this folder owns none; the
  file says what each call reports.
- `include/app/clock.h` — the interval between two frames, as a value. Its
  header says why it takes the reading rather than making one, why `elapsed` and
  `step` are separate numbers and what confusing them costs, and what the first
  tick reports.
