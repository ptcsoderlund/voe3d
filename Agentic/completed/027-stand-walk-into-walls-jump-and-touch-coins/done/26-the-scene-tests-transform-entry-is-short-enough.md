# 26 — The scene tests' transform entry is short enough
folder: scene/tests
decisions: 0168

## Change
`checks.sh --all` finds the `transform.c` entry of `scene/tests/tests.md` is 322 characters;
the cap is 300. Only that entry changes; no test, no other entry.

- `scene/tests/tests.md` — the `transform.c` entry shortened under 300 characters, keeping all
  five points: the matrix order, an intent landing when the system runs, a rotation arriving
  unit length (a zero one leaving the row alone), the field list matching the compiler's
  layout, and a remembered step blended back by a lag.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder scene/tests` prints `FINDINGS: 0`.
