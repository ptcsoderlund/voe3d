# base

The public headers, one entry each; the fuller account of every one of these
stays on `base/base.md`.

- `arena.h` — the arena the engine's working memory comes from.
- `assert.h` — the two assert macros, and which survives a release build.
- `describe.h` — a struct written once as the list of its fields, and the table
  describing them.
- `error.h` — the recoverable-failure codes and the two shapes a failing
  function takes.
- `imported.h` — the mark on engine data a project's code library reads from
  another module.
- `report.h` — the one call a recoverable problem is reported through.
- `samples.h` — a run of measurements: how many, the average, the worst.
- `version.h` — the placeholder that proves the folder builds.
