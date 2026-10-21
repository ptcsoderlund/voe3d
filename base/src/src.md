# src

`base`'s implementation, one file per header in `include/base/`, plus
`report_line.h` and `report_state.h`, which are internal to this folder.

- `assert.c` — the one function both macros expand to.
- `arena.c` — the blocks behind the arena, and the pointer bump.
- `error.c` — one phrase per code.
- `report.c` — the line composed on the stack and written in one call, and the
  per-thread subjects and kept first error, the subjects before its message,
  saved and loaded whole by `report_state.h`'s two calls.
- `report_line.h` — the composition on its own, subjects between module and
  message, so the test can read the line.
- `report_state.h` — one thread's subjects and kept error, saved and loaded
  whole, so a pool task runs under its caller's subjects (ADR-0448).
- `samples.c` — the three lines of arithmetic behind `include/base/samples.h`.
- `version.c` — the placeholder's one function, and the C23 assertion.
- `workers.c` — the pool's threads, one queue of runs under a lock, and each
  task between a report state's save and load.
