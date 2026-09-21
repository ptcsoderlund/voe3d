# 02 — component.h's header comes under the cap
folder: ecs
decisions: 0168, 0210

## Change
Comments and `.md` entries only, by 0210's method. Bring each header below to 55 lines or fewer as `checks.sh` counts them:

- `ecs/include/ecs/component.h` — header 137 lines today.

Read each header, then each declaration or definition a paragraph moves above. Then the entries for these files in the `.md` pages under `ecs/` that list them (`grep -rn --include='*.md' <file name> ecs/`): change one only if it points at the header for text that moved.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder ecs` exits 0; and 0210's token check prints nothing.
