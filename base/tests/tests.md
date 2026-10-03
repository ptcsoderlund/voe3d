# tests

One plain C program per `base` module, found by the build, each checking that
module's promises from outside.

- `arena.c` — the arena's promises, checked from outside.
- `debug_assert.c` — that under `NDEBUG` a debug assert's expression still
  counts as used and never runs. Its header says why `NDEBUG` is defined inside
  the test.
- `describe.c` — that a table's offsets, kind, shape and count are the
  compiler's own, on a struct padded so that any worked out by hand would be
  wrong, and on a second struct proving every rank from 0 to 7. Its header says
  why the switch is turned on inside the test.
- `report.c` — each level's word, the line's exact shape, and that the cut
  falls exactly at the capacity. Its header says why the boundary is the test.
- `samples.c` — that an empty run reads as noughts, that the average and the
  worst are over exactly what was added, and that a reset does not leave the
  worst behind. Its header says why that last one is the failure worth a test.
