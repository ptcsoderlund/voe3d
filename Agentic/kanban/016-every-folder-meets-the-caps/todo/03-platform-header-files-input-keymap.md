# 03 — The file, input and keymap headers come under the cap
folder: platform
decisions: 0168, 0210

## Change
Comments and `.md` entries only, by 0210's method. Bring each header below to 55 lines or fewer as `checks.sh` counts them:

- `platform/include/platform/file.h` — header 73 lines today.
- `platform/include/platform/input.h` — header 85 lines today.
- `platform/src/keymap.h` — header 77 lines today.

Read each header, then each declaration or definition a paragraph moves above. Then the entries for these files in the `.md` pages under `platform/` that list them (`grep -rn --include='*.md' <file name> platform/`): change one only if it points at the header for text that moved.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder platform` prints exactly 2 FINDING lines, the header caps of `platform/src/window_wayland.c`, `platform/src/window_win32.c`, untouched by this card, and none of its own; its product check (build and the folder's tests) passes; and 0210's token check prints nothing.
