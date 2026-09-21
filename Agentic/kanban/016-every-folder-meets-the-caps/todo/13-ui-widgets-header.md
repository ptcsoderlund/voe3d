# 13 — widgets.h's header comes under the cap
folder: ui
decisions: 0168, 0210

## Change
Comments and `.md` entries only, by 0210's method. Bring each header below to 55 lines or fewer as `checks.sh` counts them:

- `ui/include/ui/widgets.h` — header 210 lines today.

Read each header, then each declaration or definition a paragraph moves above. Find each declaration a paragraph explains by `grep -n` and read only around it. Then the entries for these files in the `.md` pages under `ui/` that list them (`grep -rn --include='*.md' <file name> ui/`): change one only if it points at the header for text that moved.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder ui` prints exactly 6 FINDING lines, the header caps of `ui/src/layout.c`, `ui/tests/layout.c`, `ui/include/ui/colour.h`, `ui/include/ui/theme.h`, `ui/src/button.c`, `ui/src/widgets.c`, untouched by this card, and none of its own; its product check (build and the folder's tests) passes; and 0210's token check prints nothing.
