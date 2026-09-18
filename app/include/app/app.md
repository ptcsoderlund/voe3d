# app

`app`'s public headers: the startup, frame and capture calls a program repeats,
and the clock that measures the interval between two frames.

- `app.h` — the whole public surface: the two startups, frame open, draw open and
  close, a capture, and the window and device to reach past them. Its header
  shows the dozen-line loop a program writes, says why there is no loop, callback
  or function pointer in this folder and what that buys, lists by name what is
  not here and is not coming, says which of the two arenas is kept and which is
  startup's scratch, and says what a frame reports and what the window is when a
  program opened with no display.
- `clock.h` — the interval between two frames, as a value. Its header says why it
  takes the reading rather than making one, why `elapsed` and `step` are separate
  numbers and what confusing them costs, and what the first tick reports.
