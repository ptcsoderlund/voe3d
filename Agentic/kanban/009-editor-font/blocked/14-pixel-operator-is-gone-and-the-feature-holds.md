# 14 — Pixel Operator's last name is gone, and the feature's test holds
folder: .
decisions: 0168, 0185, 0177
read: feature.md

## Change
`.gitattributes`: delete the line `text/fonts/PixelOperator-LICENSE.txt	binary` and, if it is left
with nothing under it, the comment "Third-party licence texts are stored exactly…" above it.

## Done when
- `git grep -n -i -e "pixel.\?operator" -e PixelOperator -- ':!history' ':!Agentic'` prints only
  `theme/tests/theme.c`'s lines that feed `font=pixel_operator` (the fallback test, step 4).
- `git diff --stat main -- dev/` prints nothing: the dev program still draws in Oxanium (step 6).
- `XDG_CONFIG_HOME=$(mktemp -d) ./build/debug/editor/voe_editor --capture <scratch>/editor.png --size 1280x800`
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

## Blocked
The `.gitattributes` change is made, the grep prints only `theme/tests/theme.c:44,105`, the capture exits 0 and shows every panel in Oxanium, and `checks.sh --all` gives FINDINGS: 0. But `checks.sh --folder .` cannot pass for the repository root: it tries to build target `voe_.`, flags `.gitattributes` as outside `./`, and reads `README.md` as the root's table of contents (20 findings the card does not cover). Separately, `git diff --stat main -- dev/` is not empty, because dev/ changed in features 006 and 008 (merged into this branch before 009); 009 itself never touched dev/, which still asks for `VOE_TEXT_TYPEFACE_OXANIUM`. Unblock by letting `checks.sh` handle folder `.` (or naming this card's folder differently) and by restating the dev criterion as "no 009 commit touches dev/" (`git log 8487cfe~1..HEAD -- dev/` is empty).
