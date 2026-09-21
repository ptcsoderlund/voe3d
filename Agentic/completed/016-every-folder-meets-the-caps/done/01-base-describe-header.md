# 01 — describe.h's header comes under the cap
folder: base
decisions: 0168, 0210

## Change
Comments and `.md` entries only, by 0210's method. Bring each header below to 55 lines or fewer as `checks.sh` counts them:

- `base/include/base/describe.h` — header 94 lines today.

Read each header, then each declaration or definition a paragraph moves above. Then the entries for these files in the `.md` pages under `base/` that list them (`grep -rn --include='*.md' <file name> base/`): change one only if it points at the header for text that moved.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder base` exits 0; and 0210's token check prints nothing.
