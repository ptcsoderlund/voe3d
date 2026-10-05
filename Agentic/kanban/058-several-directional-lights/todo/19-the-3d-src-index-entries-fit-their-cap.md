# 19 — The 3d src index entries fit their cap
folder: 3d/src
after: none
decisions: 0168

## Change
`3d/src/src.md`: the entries `draw_system.c` (312 characters), `draw_light_blockers.c` (349) and
`draw_bounce.c` (305) are over the 300-character cap. Make each one sentence under 300; detail that
is dropped and not already in the file's header comment goes into that header
(`3d/src/draw_system.c`, `3d/src/draw_light_blockers.c`, `3d/src/draw_bounce.c`), comments only.

## Done when
`checks.sh --folder 3d/src` prints no FINDING naming `src.md`.
