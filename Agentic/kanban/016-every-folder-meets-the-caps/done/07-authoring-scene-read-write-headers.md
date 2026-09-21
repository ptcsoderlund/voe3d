# 07 — The scene reader's and writer's headers come under the cap
folder: authoring
decisions: 0168, 0210

## Change
Comments and `.md` entries only, by 0210's method. Bring each header below to 55 lines or fewer as `checks.sh` counts them:

- `authoring/include/authoring/scene_read.h` — header 72 lines today.
- `authoring/include/authoring/scene_write.h` — header 80 lines today.

Read each header, then each declaration or definition a paragraph moves above. Any nesting-limit reasoning stays in the header (rule 14). Then the entries for these files in the `.md` pages under `authoring/` that list them (`grep -rn --include='*.md' <file name> authoring/`): change one only if it points at the header for text that moved.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder authoring` exits 0; and 0210's token check prints nothing.
