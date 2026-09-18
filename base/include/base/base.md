# base

`base`'s public headers: memory, containers, strings, assert and the description
of a struct's fields. Any other folder may include these; nothing here knows an
operating system.

- `assert.h` — the two assert macros. Its header says which one survives a
  release build, why it is that one, and where the line between an assert and a
  recoverable failure runs.
- `arena.h` — the arena the engine's working memory comes from. Its header
  carries the one thing a caller can get wrong: two pushes are not guaranteed to
  be next to each other.
- `error.h` — the recoverable-failure codes, and the two shapes a failing
  function takes. Its header says why the codes are categories and never
  incidents.
- `report.h` — the one call a recoverable problem is reported through, at a
  warning or an error, and the first error's message kept per thread since the
  last clear (ADR-0160) so a caller can read it back. Its header says what a
  report is for, the four things a writer gets wrong: the module, the newline,
  the file and line that are kept but not printed, and the length a line is cut
  at — and what is kept, why the first, why per thread, and when to clear.
- `samples.h` — a run of measurements, and the three things anything asks one:
  how many, the average, the worst. Its header says why it is a period and not a
  sliding window, and why `worst` presumes a direction.
- `describe.h` — a struct written once as the list of its fields, and the table
  that says what each one is and where it lives. Its header says why the
  declaring folder supplies the type, what the build refuses, what the switch
  leaves out, and what marking a field read-only does and does not mean.
- `version.h` — the placeholder that proves the folder builds.
