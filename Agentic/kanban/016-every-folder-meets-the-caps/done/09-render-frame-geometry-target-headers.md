# 09 — The frame, geometry and target sources' headers come under the cap
folder: render
decisions: 0168, 0210

## Change
Comments and `.md` entries only, by 0210's method. Bring each header below to 55 lines or fewer as `checks.sh` counts them:

- `render/src/frame.c` — header 175 lines today.
- `render/src/geometry.c` — header 67 lines today.
- `render/src/target.c` — header 87 lines today.

Read each header, then each declaration or definition a paragraph moves above. Find each function a paragraph explains by `grep -n` for its name and read only around it. Then the entries for these files in the `.md` pages under `render/` that list them (`grep -rn --include='*.md' <file name> render/`): change one only if it points at the header for text that moved.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder render` prints exactly 2 FINDING lines, the header caps of `render/tests/elements.c`, `render/tests/offscreen.c`, untouched by this card, and none of its own; its product check (build and the folder's tests) passes; and 0210's token check prints nothing.
