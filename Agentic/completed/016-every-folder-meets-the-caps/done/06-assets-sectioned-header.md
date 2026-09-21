# 06 — sectioned.h's header comes under the cap
folder: assets
decisions: 0168, 0210

## Change
Comments and `.md` entries only, by 0210's method. Bring each header below to 55 lines or fewer as `checks.sh` counts them:

- `assets/include/assets/sectioned.h` — header 93 lines today.

Read each header, then each declaration or definition a paragraph moves above. Its nesting limit and why it refuses a file past it stay in the header (rule 14). Then the entries for these files in the `.md` pages under `assets/` that list them (`grep -rn --include='*.md' <file name> assets/`): change one only if it points at the header for text that moved.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder assets` exits 0; and 0210's token check prints nothing.
