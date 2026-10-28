# 0490 — A burst task's first error is carried to the caller in its record
date: 2026-10-28
by: planner

## Decision
ADR-0160's first error is per thread; 0487's tasks run on workers. So:

1. **`base/report.h` gains `voe_base_report_error_keep(text)`**: on the calling thread, when no
   error is kept since the last clear, the text becomes the kept first error, under the same
   cut; otherwise nothing. It prints nothing: the worker's report already did.
2. **A task clears its thread's first error before its work**, and when it fails copies
   `voe_base_report_error_first()` into its own record, in the record's arena.
3. **The caller keeps the records' texts in task order** after the burst, where it reads the
   failures today (the batch's put, a material's fold), so the first failure in order is the
   one a program reads, as it was when the work ran on one thread.

## Reasoning
The editor's notice and the `game/models` and `game/models_levels` tests read the first error on
the thread that called the update; with the work on workers it stayed NULL. Carrying the text in
the record keeps state with its owner (no lock, no shared slot), and keeping in order makes the
result independent of which worker finished first. A keep that never prints avoids saying the
cause twice (0445).

## Replaces
Nothing; extends ADR-0160 to 0487's bursts.
