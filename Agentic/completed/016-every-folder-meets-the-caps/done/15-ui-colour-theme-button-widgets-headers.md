# 15 — The last four ui headers come under the cap
folder: ui
decisions: 0168, 0210, 0211

## Change
Comments and `.md` entries only, by 0210's method. Bring each header below to 55 lines or fewer as `checks.sh` counts them:

- `ui/include/ui/colour.h` — header 64 lines today.
- `ui/include/ui/theme.h` — header 77 lines today.
- `ui/src/button.c` — header 93 lines today.
- `ui/src/widgets.c` — header 71 lines today.

Read each header, then each declaration or definition a paragraph moves above. Then the entries for these files in the `.md` pages under `ui/` that list them (`grep -rn --include='*.md' <file name> ui/`): change one only if it points at the header for text that moved.

Two `ui/ui.md` findings no other card takes, both by 0211: its opening (986 characters against 400) keeps what the folder is for, the rest going to the header that owns each claim (the theme mechanism to `include/ui/widgets.h`, the palette to `include/ui/theme.h`, where they do not already say it); and its `include/ui/slider.h` entry (572 against 300) says what the file holds, what follows "Its header says" being in `slider.h`'s header already or moving there, which stays at 55 lines or fewer.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder ui` exits 0; and 0210's token check prints nothing.
