# 64 — The scene transform test entry fits
folder: scene/tests
after: none
decisions: 0168

## Change
- `scene/tests/tests.md`: the `transform.c` entry is one sentence of 300 characters or fewer (now 391).
  The header of `scene/tests/transform.c` already makes every point the entry carries, so change only the
  entry. Do not open the rest of that file. It is 911 lines.

## Done when
`checks.sh --folder scene/tests` reports no finding in `scene/tests`.
