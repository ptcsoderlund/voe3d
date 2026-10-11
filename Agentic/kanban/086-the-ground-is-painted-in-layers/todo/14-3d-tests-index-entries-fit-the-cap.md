# 14 — 3d/tests index entries fit the cap
folder: 3d/tests
after: none
decisions: 0168

## Change
Two entries in `3d/tests/tests.md` are over the 300-character entry cap:
`draw_terrain.c` (405) and `models_landscape.c` (359). Cut each to one
sentence saying what the test file proves. Anything dropped that is not
already in the header comment of `3d/tests/draw_terrain.c` or
`3d/tests/models_landscape.c` moves into that header (keep the header under
60 lines). Docs only; no code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d/tests` prints no
FINDING naming `3d/tests/tests.md`.
