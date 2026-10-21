# 49 — A report's subjects and kept error cross to another thread
folder: base
after: none
decisions: 0168, 0445, 0448

## Change
0448 point 1 needs a task on a pool thread to report under its caller's subjects (0445), and
the caller to read back what a task kept. Card 50's pool is the only user; this stays inside
`base`.

- `base/src/report_state.h` — new, internal to `base` as `report_line.h` is. A struct holding one
  thread's report state: its subjects (pointers, up to `VOE_BASE_REPORT_SUBJECTS`, and how many)
  and its kept first error (the text, as kept, and whether there is one). Two calls:
  `voe_base_report_state_save(struct voe_base_report_state *out)` copies the calling thread's
  into `out`; `voe_base_report_state_load(const struct voe_base_report_state *in)` makes the
  calling thread's exactly `in`, subjects and kept error both. Header: why it exists (a pool
  task runs under its caller's subjects and hands back its kept error, 0448), that subjects are
  copied as pointers so the caller's strings must outlive the task, and that nothing outside
  `base` includes it.
- `base/src/report.c` — the two calls over its `thread_local` subjects and kept text. Header: the
  state can be saved and loaded whole, for the pool.
- `base/src/src.md` — an entry for `report_state.h`; the `report.c` entry names save and load.
- `base/tests/report.c` — new checks (the test may include `../src/report_state.h`, as it does
  `report_line.h`): with two subjects pushed and an error reported, a state saved, then on a
  second thread (`thrd_create`) loaded: an error reported there prints and keeps both subjects
  before its message, and `voe_base_report_error_first()` there is the loaded kept text until a
  clear; a state saved with nothing pushed and nothing kept, loaded over a thread that had both,
  leaves it with neither.
- `base/tests/tests.md` — the `report.c` entry names the checks.

## Done when
`ctest --test-dir build/debug -R '^base/report$'` passes with the new checks.
