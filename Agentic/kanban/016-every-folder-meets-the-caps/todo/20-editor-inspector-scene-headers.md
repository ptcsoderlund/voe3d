# 20 — The inspector's and scene's headers come under the cap
folder: editor
decisions: 0168, 0210

## Change
Comments and `.md` entries only, by 0210's method. Bring each header below to 55 lines or fewer as `checks.sh` counts them:

- `editor/src/inspector.h` — header 84 lines today.
- `editor/src/scene.h` — header 86 lines today.

Read each header, then each declaration or definition a paragraph moves above. Then the entries for these files in the `.md` pages under `editor/` that list them (`grep -rn --include='*.md' <file name> editor/`): change one only if it points at the header for text that moved.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder editor` prints exactly 2 FINDING lines, the header caps of `editor/src/main.c`, `editor/src/themes.h`, untouched by this card, and none of its own; its product check (build and the folder's tests) passes; and 0210's token check prints nothing.
