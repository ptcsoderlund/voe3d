# 13 — The 3d tests index entry for light_blockers.c fits its cap
folder: 3d/tests
after: none
decisions: 0168

## Change
`3d/tests/tests.md`: the entry for `light_blockers.c` (line ~42) is 330
characters, cap 300. Shorten it to one sentence under 300 characters naming
what the test proves. Any detail cut that is not already in the top comment of
`3d/tests/light_blockers.c` goes there instead (keep that comment under 60
lines). No code changes.

## Done when
`checks.sh --folder 3d/tests` reports no finding on `tests.md`.
