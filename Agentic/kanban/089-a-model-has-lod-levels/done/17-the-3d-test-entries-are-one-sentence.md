# 17 — The 3d tests' index entries are one sentence
folder: 3d/tests
after: none
decisions: 0168

## Change
`3d/tests/tests.md`: two entries are over the 300-character cap —
`model_bake.c` (347) and `model_levels.c` (353). Cut each to one sentence
under 260 characters naming what the test proves at the top level (vertex
colours and `_LOD<n>` levels baked; a levels file made, read, staled and a
view's level chosen), keeping "Needs no graphics card." The details dropped
must be in the header comment of `3d/tests/model_bake.c` and
`3d/tests/model_levels.c` respectively; add there any the header does not
already say, keeping each header within its cap. No code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure` prints no
FINDING line naming `3d/tests/`, and
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines 3d/tests/model_bake.c 3d/tests/model_levels.c`
prints no "over cap".
