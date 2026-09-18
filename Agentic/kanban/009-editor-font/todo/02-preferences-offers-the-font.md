# 02 — Preferences offers the font, and the feature's test holds
folder: editor
decisions: 0168, 0179, 0177
read: feature.md

## Change
`src/preferences.h` / `.c`: beside the list of themes, the panel shows a font group. It has three rows, in this
order: `Theme's own`, `Pixel Operator`, `Oxanium`. Each row has a Choose button, and the row equal to
`themes->font_choice` is marked the same way the theme in force is. Draw these rows in the theme in force, not
pushed in any entry's palette. Record their buttons in a new `voe_ui_node font_buttons[3]`. Add
`VOE_EDITOR_PREFERENCES_FONT` to the action enum; its `index` is the `voe_editor_font_choice`. Update the header:
what the panel lists, and why the font rows are drawn in the theme in force.

`src/interface.c`: on `VOE_EDITOR_PREFERENCES_FONT`, call `voe_editor_themes_font_choose` (a false return
leaves a notice, as a failed theme Choose does), then `voe_ui_font_set(ui, voe_editor_themes_palette(themes)->font)`
and `voe_ui_theme_set(ui, voe_editor_themes_palette(themes))`. Do not touch the session: it is not an unsaved
change and it does not disarm anything.

`src/interface.h`: raise the node and element budgets by what the font group adds, itemized in its comment the
way the browser's are.

Update `src/src.md` (preferences entry) and `editor.md` (Preferences also offers the font).

## Done when
`checks.sh --all` exits 0 (step 8). `git diff --stat refs/heads/dev -- dev/` prints nothing, so the dev program still draws in
Oxanium (step 7). Card 01's five captures still hold (step 6), and reading `oxanium.png` shows Oxanium
(ADR-0177).

For the human (steps 1–5 and 7), with `./build/debug/editor/voe_editor` and `feature.md`'s `## How to test`:
1. With no `~/.config/voe3d/theme` or `font`: the top bar, Scene, Inspector, the file browser and Preferences are
   all in Pixel Operator. Choosing Near white keeps Pixel Operator.
2. Preferences shows the font group next to the themes (theme's own, Pixel Operator, Oxanium), with the one in
   use marked.
3. Choosing Oxanium redraws every panel in Oxanium at once, under Near black and under Near white. Choosing the
   theme's own font puts it back. The scene does not change and the bar shows no unsaved mark.
4. Choose Oxanium, close the editor and start it again: the panels are in Oxanium.
5. While Oxanium is chosen, each theme row in Preferences still shows its own theme's font.
7. `./build/debug/dev/voe_dev` still draws its text in Oxanium.
