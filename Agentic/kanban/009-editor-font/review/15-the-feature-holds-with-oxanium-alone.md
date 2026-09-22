# 15 — The feature holds with Oxanium alone
folder: theme
decisions: 0168, 0185, 0177
read: feature.md

## Change
None to write. `.gitattributes` already lost Pixel Operator's licence line (commit 43f6bf4);
`theme/tests/theme.c` keeps its `font=pixel_operator` lines as the fallback test (step 4). This card
proves `feature.md`'s `## How to test`. If a proof below fails, fix it in `theme/` only; a fault in
another folder is a `## Blocked`.

## Done when
- `git grep -n -i -e "pixel.\?operator" -e PixelOperator -- ':!history' ':!Agentic'` prints only
  lines of `theme/tests/theme.c` (step 4).
- `git log --oneline 6c795e1..HEAD -- dev/` prints nothing: no 009 commit touched dev/, which still
  asks for `VOE_TEXT_TYPEFACE_OXANIUM` (step 6).
- `XDG_CONFIG_HOME=$(mktemp -d) ./build/debug/editor/voe_editor --capture "$(mktemp -d)/editor.png" --size 1280x800`
  exits 0, and reading the PNG shows every panel in Oxanium (step 5, ADR-0177).
- `checks.sh --all` exits 0 (step 7).

For the human, with `./build/debug/editor/voe_editor` and `feature.md`'s `## How to test`:
1. Every panel (top bar, Scene list, Inspector, file browser, Preferences) is in Oxanium; choose Near
   white and it still is.
2. Preferences lists the themes and offers no font.
3. Make the window smaller in steps: no letter loses a stroke and the text stays readable; full screen
   looks as good as before.
4. Put a theme file with `font=pixel_operator` in `~/.config/voe3d/themes/` and choose it: it draws in
   Oxanium, with no notice.
6. `./build/debug/dev/voe_dev` draws its text in Oxanium.
