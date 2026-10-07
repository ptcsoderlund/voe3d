# 14 — The file test reads no uninitialised byte
folder: platform/tests
after: none
decisions: 0168

## Change
Two findings in this folder.

- `platform/tests/file.c`: the static analyser (clang-analyzer, run by
  `check.cmake`) reports at line 200, in the folder-move test, that
  `got[0]` may be uninitialised after `read_back` (line 74) returned —
  it cannot see that a length of 1 means a byte was read. Initialise that
  test function's `got` buffer at its declaration (zeroed). Check the other
  test functions that read `got` after `read_back` (lines ~176, ~216,
  ~219) and initialise theirs the same way if they declare their own.
  No change to `read_back` or to what any test checks.
- `platform/tests/tests.md`: the entry `file.c` is 363 characters, cap
  300. Cut it to one sentence under 300 characters (the round trip, the
  replacement, failed paths, the move cases); any detail cut that the
  header comment of `file.c` lacks goes there. Also join that header's
  over-long line "are functions of their own, run before the rest. Needs no
  window…" back into the comment's line width.

## Done when
`bash /home/ptcsoderlund/.claude/skills/checks/scripts/checks.sh --folder platform/tests` prints `FINDINGS: 0`, and
`ctest --test-dir build/debug -R '^platform/'` passes after a build.
