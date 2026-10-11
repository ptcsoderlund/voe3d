# 13 — 3d/src index entries fit the cap
folder: 3d/src
after: none
decisions: 0168

## Change
Two entries in `3d/src/src.md` are over the 300-character entry cap:
`draw_terrain.c` (342) and `models_landscape.c` (368). Cut each to one
sentence saying what the file does. Anything dropped that is not already in
the header comment of `3d/src/draw_terrain.c` or `3d/src/models_landscape.c`
moves into that header (keep the header under 60 lines). Docs only; no code
changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d/src` prints no
FINDING naming `3d/src/src.md`.
