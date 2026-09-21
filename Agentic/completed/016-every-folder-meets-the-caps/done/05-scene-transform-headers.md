# 05 — The transform headers come under the cap
folder: scene
decisions: 0168, 0210

## Change
Comments and `.md` entries only, by 0210's method. Bring each header below to 55 lines or fewer as `checks.sh` counts them:

- `scene/include/scene/transform_component.h` — header 67 lines today.
- `scene/include/scene/transform_system.h` — header 61 lines today.

Read each header, then each declaration or definition a paragraph moves above. Then the entries for these files in the `.md` pages under `scene/` that list them (`grep -rn --include='*.md' <file name> scene/`): change one only if it points at the header for text that moved.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder scene` exits 0; and 0210's token check prints nothing.
