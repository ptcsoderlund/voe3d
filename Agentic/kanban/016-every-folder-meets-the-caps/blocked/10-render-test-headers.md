# 10 — The elements and offscreen tests' headers come under the cap
folder: render
decisions: 0168, 0210

## Change
Comments and `.md` entries only, by 0210's method. Bring each header below to 55 lines or fewer as `checks.sh` counts them:

- `render/tests/elements.c` — header 111 lines today.
- `render/tests/offscreen.c` — header 71 lines today.

Read each header, then each declaration or definition a paragraph moves above. A paragraph about one claim goes above the test function that makes it. Then the entries for these files in the `.md` pages under `render/` that list them (`grep -rn --include='*.md' <file name> render/`): change one only if it points at the header for text that moved.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder render` exits 0; and 0210's token check prints nothing.

## Blocked
Both headers are now under the cap (elements.c 14 lines, offscreen.c 38) and 0210's token check prints nothing, but `checks.sh --folder render` still reports one finding this card does not cover: `render/render.md`'s opening is 1205 characters against the 400 cap. Shortening it means moving true prose out of the folder page into some file's header, which no card in 016 names; a card (or a decision on where render.md's opening prose goes) would unblock it.
