# 11 — The shadow test's index entry fits its cap
folder: 3d/tests
after: none
decisions: 0168

## Change
`3d/tests/tests.md`: the entry for `shadows.c` is 321 characters; the index
cap is 300. Cut it to one sentence: a cube under a straight-down sun shadows
the floor, near the origin and 100 km out, skipped without a graphics card.
The list of cases it drops (fill lifts the shadow, no light, a light or a cube
with `cast_shadows` off, a model, bounce off) belongs in the header of
`3d/tests/shadows.c`; check its header comment says each and add any it lacks.
No code changes.

## Done when
`checks.sh --folder 3d/tests` reports no finding for `tests.md`.
