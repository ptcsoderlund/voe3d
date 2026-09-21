# 04 — Both window backends' headers come under the cap
folder: platform
decisions: 0168, 0210

## Change
Comments and `.md` entries only, by 0210's method. Bring each header below to 55 lines or fewer as `checks.sh` counts them:

- `platform/src/window_wayland.c` — header 140 lines today.
- `platform/src/window_win32.c` — header 103 lines today.

Read each header, then each declaration or definition a paragraph moves above. The two backends are equals (0168's scope of a card): where their headers say the same thing, both keep it. `window_win32.c` does not build on Linux; the token check is its proof. Then the entries for these files in the `.md` pages under `platform/` that list them (`grep -rn --include='*.md' <file name> platform/`): change one only if it points at the header for text that moved.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder platform` exits 0; and 0210's token check prints nothing.
