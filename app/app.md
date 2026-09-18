# app

The parts every program's frame loop repeats: a window and a device opened
together — or a device alone, with no window at all — a frame opened, a draw
opened and closed, and what was drawn written out as a picture. Not the loop
itself, and not what a program decides — system order, a world, arenas, a
readout, key bindings and the present mode are all the program's.

- `include` — the public headers, in `include/app/`. See `include/app/app.md`.
- `src` — the implementation. See `src/src.md`.
- `tests` — one plain C program per module, found by the build. See
  `tests/tests.md`.
