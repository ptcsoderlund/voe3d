# 16 — theme.h's header comes under the cap
folder: theme
decisions: 0168, 0210, 0211

## Change
Comments and `.md` entries only, by 0210's method. Bring each header below to 55 lines or fewer as `checks.sh` counts them:

- `theme/include/theme/theme.h` — header 63 lines today.

Read each header, then each declaration or definition a paragraph moves above. Three lines over: tightening or one moved paragraph is enough. Then the entries for these files in the `.md` pages under `theme/` that list them (`grep -rn --include='*.md' <file name> theme/`): change one only if it points at the header for text that moved.
`theme/theme.md`'s opening is 418 characters against 400: bring it to 400 or fewer by 0211 (the ADR-0176 sentence on `render` goes to the header of the file it is about, if that header does not already say it).

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder theme` exits 0; and 0210's token check prints nothing.
