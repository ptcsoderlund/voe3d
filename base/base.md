# base

Memory, containers, strings, assert and the description of a struct's fields —
the primitives any other folder may depend on. Not maths, and nothing that knows
an operating system.

- `include` — the public headers, in `include/base/`; each is listed below by path.
- `src` — the implementation. See `src/src.md`.
- `tests` — one plain C program per module, found by the build. See `tests/tests.md`.
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
  and the table that says what each one is and where it lives, with a field's
  values optionally named for a tool. Its header says why the declaring folder
  supplies the type, what the build refuses, what the switch leaves out, and
  what marking a field read-only or naming its values does and does not mean.
- `include/base/version.h` — the placeholder that proves the folder builds.
