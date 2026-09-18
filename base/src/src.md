# src

`base`'s implementation: one file per header above it, plus the one internal
header the report and its test share. Nothing outside `base` includes anything
here.

- `assert.c` — the one function both macros expand to.
- `arena.c` — the blocks behind the arena, and the pointer bump.
- `error.c` — one phrase per code.
- `report.c` — the line composed on the stack and written in one call.
- `report_line.h` — the composition on its own, so the test can read the line.
- `samples.c` — the three lines of arithmetic behind the header above.
- `version.c` — the placeholder's one function, and the C23 assertion.
