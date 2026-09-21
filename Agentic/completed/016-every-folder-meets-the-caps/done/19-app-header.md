# 19 — app.h's header comes under the cap
folder: app
decisions: 0168, 0210

## Change
Comments and `.md` entries only, by 0210's method. Bring each header below to 55 lines or fewer as `checks.sh` counts them:

- `app/include/app/app.h` — header 87 lines today.

Read each header, then each declaration or definition a paragraph moves above. Then the entries for these files in the `.md` pages under `app/` that list them (`grep -rn --include='*.md' <file name> app/`): change one only if it points at the header for text that moved.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder app` exits 0; and 0210's token check prints nothing.
