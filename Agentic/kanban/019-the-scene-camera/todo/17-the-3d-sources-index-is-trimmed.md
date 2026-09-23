# 17 — The 3d sources' index entries fit the cap
folder: 3d/src
decisions: 0168

## Change
In `3d/src/src.md` the `pick.c` entry (317 characters) and the `draw_system.c` entry (347)
are over the 300-character cap. Cut each to one sentence under 300 characters naming what
the file does: for `pick.c`, the pixel's ray from the inverted matrices and the walk over
shapes then camera marker boxes; for `draw_system.c`, the camera's view and sun, the solid
and blended passes, the marker with the solids, the outline and gizmo. Detail cut from an
entry that the file's header does not already carry goes into that header (`3d/src/pick.c`,
`3d/src/draw_system.c`, header comment only; measure it with
`checks.sh --header-lines`). No code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d/src` prints `FINDINGS: 0`.
