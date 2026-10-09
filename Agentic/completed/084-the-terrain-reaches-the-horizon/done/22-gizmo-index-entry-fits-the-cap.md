# 22 — The gizmo.c index entry fits the cap
folder: 3d/tests
after: none
decisions: 0168

## Change
Doc only; no code changes.

`3d/tests/tests.md`: the entry `gizmo.c` is 336 characters, over the 300
cap. Shorten it to one sentence well under 300: rays meeting each arrow and
square and missing beside them, the shaft growing with distance and absent
behind or beside the eye, the grabs and the refusal, and the meshes' counts,
winding and marking; needs no graphics card. Drop the enumerating detail
("doubling", "two grabs and the one refusal", "moves across").

`3d/tests/gizmo.c`: read its header comment and the comments above each test
only. They already state each dropped detail; change one only if a claim is
missing. Touch no test code.

## Done when
`checks.sh --folder 3d/tests` reports no finding naming `gizmo.c`.
