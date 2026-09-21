# 11 — The font and raster headers come under the cap
folder: text
decisions: 0168, 0210

## Change
Comments and `.md` entries only, by 0210's method. Bring each header below to 55 lines or fewer as `checks.sh` counts them:

- `text/include/text/font.h` — header 96 lines today.
- `text/src/font.c` — header 71 lines today.
- `text/src/raster.h` — header 69 lines today.

Read each header, then each declaration or definition a paragraph moves above. Then the entries for these files in the `.md` pages under `text/` that list them (`grep -rn --include='*.md' <file name> text/`): change one only if it points at the header for text that moved.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder text` exits 0; and 0210's token check prints nothing.
