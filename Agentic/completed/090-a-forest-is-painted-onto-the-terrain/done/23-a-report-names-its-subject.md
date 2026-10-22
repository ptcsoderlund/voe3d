# 23 — A report names its subject
folder: base
after: none
decisions: 0168, 0443, 0445

## Change
0445 point 1: a per-thread stack of subjects every report prints and keeps before its message.

- `base/include/base/report.h` —
  - `VOE_BASE_REPORT_SUBJECTS` (4): how deep the stack goes.
  - `void voe_base_report_subject_push(const char *subject)`: the caller's string, kept by pointer
    until the matching pop, on the calling thread only. A fifth push, a NULL or an empty subject
    asserts (the caller's bug).
  - `void voe_base_report_subject_pop(void)`: drops the last pushed; popping an empty stack asserts.
  - Header points to add: what a subject is for (the file, then the part, material or picture, so a
    reader handed bytes still names them); who pushes (the code that knows; code below never names
    the path again); that the printed line is `<level>: <module>: <s1>: <s2>: <message>`; that the
    kept first error carries the same subjects before the message (amend the ADR-0160 paragraph,
    which says the kept text has no module and no level, and now has the subjects); pushes and pops
    pair within one function, like a mark and a rewind.
- `base/src/report.c` — the thread_local stack beside the kept error; push and pop; the kept message
  composed with the subjects.
- `base/src/report_line.h` — `voe_base_report_compose` writes the subjects between the module and
  the message, each followed by `: `, under the same 1024-byte cut; its comment says so.
- `base/src/src.md` — the `report.c` and `report_line.h` entries name the subjects.
- `base/tests/report.c` — new checks: a line with no subject is exactly as before; with two pushed
  it reads `error: m: a.glb: the part `x`: message`; after a pop, one subject; the kept first error
  begins `a.glb: the part `x`: `; a line past the cap still ends in `...` with subjects pushed.
- `base/tests/tests.md` — the `report.c` entry names the subjects.

## Done when
`ctest --test-dir build/debug -R '^base/report$'` passes with the new checks.
