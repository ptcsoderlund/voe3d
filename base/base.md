# base

Memory, containers, strings, assert and the description of a struct's fields —
the primitives any other folder may depend on. Not maths, and nothing that knows
an operating system.

- `include/base/assert.h` — the two assert macros. Its header says which one
  survives a release build, why it is that one, and where the line between an
  assert and a recoverable failure runs.
- `include/base/arena.h` — the arena the engine's working memory comes from.
  Its header carries the one thing a caller can get wrong: two pushes are not
  guaranteed to be next to each other.
- `include/base/error.h` — the recoverable-failure codes, and the two shapes a
  failing function takes. Its header says why the codes are categories and never
  incidents.
- `include/base/report.h` — the one call a recoverable problem is reported
  through, at a warning or an error, and the first error's message kept per
  thread since the last clear (ADR-0160) so a caller can read it back. Its
  header says what a report is for, the four things a writer gets wrong: the
  module, the newline, the file and line that are kept but not printed, and the
  length a line is cut at — and what is kept, why the first, why per thread,
  and when to clear.
- `include/base/samples.h` — a run of measurements, and the three things anything
  asks one: how many, the average, the worst. Its header says why it is a period
  and not a sliding window, and why `worst` presumes a direction.
- `include/base/describe.h` — a struct written once as the list of its fields,
  and the table that says what each one is and where it lives. Its header says
  why the declaring folder supplies the type, what the build refuses, what the
  switch leaves out, and what marking a field read-only does and does not mean.
- `include/base/version.h` — the placeholder that proves the folder builds.
- `src/assert.c` — the one function both macros expand to.
- `src/arena.c` — the blocks behind the arena, and the pointer bump.
- `src/error.c` — one phrase per code.
- `src/report.c` — the line composed on the stack and written in one call.
- `src/report_line.h` — the composition on its own, so the test can read the line.
- `src/samples.c` — the three lines of arithmetic behind the header above.
- `src/version.c` — the placeholder's one function, and the C23 assertion.
- `tests/arena.c` — the arena's promises, checked from outside.
- `tests/describe.c` — that a table's offsets, kind, shape and count are the
  compiler's own, on a struct padded so that any worked out by hand would be
  wrong, and on a second struct proving every rank from 0 to 7. Its header says
  why the switch is turned on inside the test.
- `tests/report.c` — each level's word, the line's exact shape, and that the cut
  falls exactly at the capacity. Its header says why the boundary is the test.
- `tests/samples.c` — that an empty run reads as noughts, that the average and
  the worst are over exactly what was added, and that a reset does not leave the
  worst behind. Its header says why that last one is the failure worth a test.
