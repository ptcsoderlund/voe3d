# 18 — outline.h's and shape_system.h's headers come under the cap
folder: 3d
decisions: 0168, 0210

## Change
Comments and `.md` entries only, by 0210's method. Bring each header below to 55 lines or fewer as `checks.sh` counts them:

- `3d/include/3d/outline.h` — header 73 lines today.
- `3d/include/3d/shape_system.h` — header 90 lines today.

Read each header, then each declaration or definition a paragraph moves above. Then the entries for these files in the `.md` pages under `3d/` that list them (`grep -rn --include='*.md' <file name> 3d/`): change one only if it points at the header for text that moved.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d` exits 0; and 0210's token check prints nothing.
