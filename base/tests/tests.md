# tests

One plain C program per `base` module, found by the build, each checking that
module's promises from outside.

- `arena.c` — the arena's promises, checked from outside.
- `debug_assert.c` — that under `NDEBUG` a debug assert's expression still
  counts as used and never runs. Its header says why `NDEBUG` is defined inside
  the test.
- `describe.c` — that a table's offsets, kind, shape and count are the
  compiler's own, on a struct padded so that any worked out by hand would be
  wrong and on one of every rank, and that names and ranges come back as they
  went in. Its header says why the switch is on.
- `report.c` — each level's word, the line's shape with and without subjects,
  the kept first error and the keep, and the cut exactly at the capacity; its header says
  why the boundary is the test.
- `samples.c` — that an empty run reads as noughts, that the average and the
  worst are over exactly what was added, and that a reset does not leave the
  worst behind. Its header says why that last one is the failure worth a test.
