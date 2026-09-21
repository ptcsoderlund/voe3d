# 16 — theme.h's header comes under the cap
folder: theme
decisions: 0168, 0210

## Change
Comments and `.md` entries only, by 0210's method. Bring each header below to 55 lines or fewer as `checks.sh` counts them:

- `theme/include/theme/theme.h` — header 63 lines today.

Read each header, then each declaration or definition a paragraph moves above. Three lines over: tightening or one moved paragraph is enough. Then the entries for these files in the `.md` pages under `theme/` that list them (`grep -rn --include='*.md' <file name> theme/`): change one only if it points at the header for text that moved.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder theme` exits 0; and 0210's token check prints nothing.
