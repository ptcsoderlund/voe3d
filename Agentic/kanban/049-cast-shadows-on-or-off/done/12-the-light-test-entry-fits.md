# 12 — The light test's index entry fits its cap
folder: scene/tests
after: none
decisions: 0168

## Change
`scene/tests/tests.md`: the entry for `light.c` is 357 characters; the index
cap is 300. Cut it to one sentence: the sun's registration, intents, bounces,
cast shadows and direction conversions. The detail it drops (a refused intent
keeps the row, bounces named "0" and "1", cast shadows off by default and on
in the unsaid row, +Z among the conversions) belongs in the header of
`scene/tests/light.c`; check its header comment says each and add any it
lacks. No code changes.

## Done when
`checks.sh --folder scene/tests` reports no finding for `tests.md`.
