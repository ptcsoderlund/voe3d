# 14 — layout.c's and its test's headers come under the cap
folder: ui
decisions: 0168, 0210

## Change
Comments and `.md` entries only, by 0210's method. Bring each header below to 55 lines or fewer as `checks.sh` counts them:

- `ui/src/layout.c` — header 153 lines today.
- `ui/tests/layout.c` — header 74 lines today.

Read each header, then each declaration or definition a paragraph moves above. Both files are long (1354 and 2294 lines); find each function by `grep -n` and read only around it. In the test, a paragraph about one claim goes above the test function that makes it. Then the entries for these files in the `.md` pages under `ui/` that list them (`grep -rn --include='*.md' <file name> ui/`): change one only if it points at the header for text that moved.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder ui` prints exactly 8 FINDING lines, the header caps of `ui/include/ui/colour.h`, `ui/include/ui/theme.h`, `ui/src/button.c`, `ui/src/widgets.c`, and `ui/ui.md`'s opening and its entries for `include/ui/colour.h`, `include/ui/slider.h`, `include/ui/theme.h`, untouched by this card, and none of its own; its product check (build and the folder's tests) passes; and 0210's token check prints nothing.
