# 12 — layout.h's header comes under the cap
folder: ui
decisions: 0168, 0210

## Change
Comments and `.md` entries only, by 0210's method. Bring each header below to 55 lines or fewer as `checks.sh` counts them:

- `ui/include/ui/layout.h` — header 292 lines today.

Read each header, then each declaration or definition a paragraph moves above. The longest header in the tree, and step 5 of feature.md's `## How to test`: the human opens this file and must find a short top comment that says what it is for, with the reasoning above the functions and types it explains. The usage sketch may stay, shortened if it must. Find each declaration by `grep -n` and read only around it. Then the entries for these files in the `.md` pages under `ui/` that list them (`grep -rn --include='*.md' <file name> ui/`): change one only if it points at the header for text that moved.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder ui` prints exactly 7 FINDING lines, the header caps of `ui/include/ui/widgets.h`, `ui/src/layout.c`, `ui/tests/layout.c`, `ui/include/ui/colour.h`, `ui/include/ui/theme.h`, `ui/src/button.c`, `ui/src/widgets.c`, untouched by this card, and none of its own; its product check (build and the folder's tests) passes; and 0210's token check prints nothing.
