# 10 — A thread keeps an error carried from another
folder: base
after: none
decisions: 0168, 0490

## Change
0490 point 1. An addition to `base/report.h`; no caller changes here.

- `base/include/base/report.h`:
  - add `void voe_base_report_error_keep(const char *text);` beside
    `voe_base_report_error_first`: on the calling thread, when nothing is kept since the last
    clear, `text` becomes the kept first error, cut as a kept report is; when one is kept,
    nothing. Prints nothing. A NULL text asserts.
  - its comment, and the header's "A PROGRAM CAN READ THE FIRST ERROR BACK" paragraph: the keep
    is how code that runs work on other threads (a burst, 0487) carries a worker's first error
    back to the thread that started it, in its own order (0490); the "nothing below a program
    reads it" line allows that one use.
- `base/src/report.c` — the keep, writing the same per-thread kept text the error path writes,
  through the same cut.
- `base/tests/report.c` — checks in `main`: a keep with nothing kept is read back by
  `voe_base_report_error_first`; a second keep, or a keep after a `VOE_BASE_ERROR`, leaves the
  first; a clear then a keep keeps the new text; a text over the capacity is cut as a kept
  error is.
- `base/base.md` — the `include/base/report.h` entry adds the keep that carries an error
  across threads. `base/src/src.md` `report.c` and `base/tests/tests.md` `report.c` entries
  name the keep.

## Done when
The `base/report` test passes with the new checks.
