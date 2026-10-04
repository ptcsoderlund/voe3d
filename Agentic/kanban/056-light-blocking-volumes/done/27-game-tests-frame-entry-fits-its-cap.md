# 27 — The game tests index entry for frame.c fits its cap
folder: game/tests
after: none
decisions: 0168

## Change
`game/tests/tests.md`: the entry for `frame.c` (line ~8) is 302 characters,
cap 300. Shorten it to one sentence under 300 characters naming what the
test proves (e.g. group the light blocker and Wall shadow cases into one
phrase). Any detail cut that is not already in the top comment of
`game/tests/frame.c` goes there instead (keep that comment under 60 lines).
No code changes.

## Done when
`checks.sh --folder game/tests` reports no finding on `tests.md`.
