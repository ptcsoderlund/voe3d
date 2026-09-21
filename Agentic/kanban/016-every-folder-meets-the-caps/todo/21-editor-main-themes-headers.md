# 21 — The editor's main.c and themes.h headers come under the cap
folder: editor
decisions: 0168, 0210

## Change
Comments and `.md` entries only, by 0210's method. Bring each header below to 55 lines or fewer as `checks.sh` counts them:

- `editor/src/main.c` — header 66 lines today.
- `editor/src/themes.h` — header 85 lines today.

Read each header, then each declaration or definition a paragraph moves above. `main.c` is 828 lines; find each function by `grep -n` and read only around it. Then the entries for these files in the `.md` pages under `editor/` that list them (`grep -rn --include='*.md' <file name> editor/`): change one only if it points at the header for text that moved.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder editor` exits 0; and 0210's token check prints nothing.
