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
- `src/samples.c` — the three lines of arithmetic behind the header above.
- `src/version.c` — the placeholder's one function, and the C23 assertion.
- `tests/arena.c` — the arena's promises, checked from outside.
- `tests/describe.c` — that a table's offsets are the compiler's own, on a struct
  padded so that any worked out by hand would be wrong. Its header says why the
  switch is turned on inside the test.
- `tests/samples.c` — that an empty run reads as noughts, that the average and
  the worst are over exactly what was added, and that a reset does not leave the
  worst behind. Its header says why that last one is the failure worth a test.
