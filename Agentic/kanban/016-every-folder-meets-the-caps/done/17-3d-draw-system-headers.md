# 17 — The draw system's headers come under the cap
folder: 3d
decisions: 0168, 0210

## Change
Comments and `.md` entries only, by 0210's method. Bring each header below to 55 lines or fewer as `checks.sh` counts them:

- `3d/include/3d/draw_system.h` — header 224 lines today.
- `3d/src/draw_system.c` — header 77 lines today.

Read each header, then each declaration or definition a paragraph moves above. Where the header and the source explain the same step, 0210 says which one keeps it. Then the entries for these files in the `.md` pages under `3d/` that list them (`grep -rn --include='*.md' <file name> 3d/`): change one only if it points at the header for text that moved.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d` prints exactly 2 FINDING lines, the header caps of `3d/include/3d/outline.h`, `3d/include/3d/shape_system.h`, untouched by this card, and none of its own; its product check (build and the folder's tests) passes; and 0210's token check prints nothing.
