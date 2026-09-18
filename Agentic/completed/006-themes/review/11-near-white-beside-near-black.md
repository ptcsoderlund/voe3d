# 11 — Near white beside Near black, and the feature's test holds
folder: editor
decisions: 0168, 0172, 0177, 0178
read: feature.md

## Change
Card 10's change (live editing, and a refused save keeps the last good theme) is already committed in
`editor/src/themes.{c,h}` and `main.c`. It only failed on a `CLAUDE.md` heading, which is now gone. Do not
redo it.

`src/themes.c` / `.h`: the list starts with two built-in entries, no longer one (ADR-0178). Entry 0 is named
`Near black` and uses `voe_ui_theme_default_inputs()`. Entry 1 is named `Near white` and uses the same inputs
with `mode = VOE_UI_THEME_MODE_LIGHT`. Both use Oxanium, have `file`, `bytes` and `arena` NULL, and are skipped
by `voe_editor_themes_check`. Loading reads the remembered line this way:
- empty or absent: entry 0;
- `near_white`: entry 1;
- anything else: a file name, as now. If that file is missing or refused, fall back to entry 0 and answer false.

`voe_editor_themes_choose` writes `near_white` for entry 1 and an empty line for entry 0. Update the header:
the list's first two entries, and what the remembered line holds. Also fix any `Built-in` wording and every
"index 0 is the built-in" assumption in `preferences.c`, `interface.c` and `main.c`: say "a theme with no file".
Update `src/src.md`'s `themes.h` entry.

## Done when
`checks.sh --all` exits 0. Then create a scratch folder whose `voe3d/theme` holds `near_white`. With
`XDG_CONFIG_HOME` set to it, `./build/debug/editor/voe_editor --capture <scratch>/white.png` writes a PNG, and
reading it shows light panels with dark text (ADR-0177).

For the human, from `feature.md`'s `## How to test`, with the editor started from
`./build/debug/editor/voe_editor`:
1. It opens in Near black. The selected Scene row is in the accent with no `> `. A pressed button and a dragged
   number show the accent.
2. Preferences in the top bar opens the list with the theme in use marked. Close and Escape both close it.
3. With no theme files, Preferences lists Near black and Near white. Choosing Near white turns every panel
   light with dark readable text and the same blue accent. Choosing Near black turns it back. A restart keeps
   the choice.
4. A `.theme` file dropped in `~/.config/voe3d/themes/` appears in Preferences after a restart. Choosing it
   restyles every panel, and a restart keeps it.
5. `mode=light` makes the editor light and readable. Changing only `accent` recolours the accent. The two
   scalars change how far surfaces and text stand apart.
6. Saving the chosen file shows the change within about a second.
7. Saving a mistake keeps the last good theme and shows a notice with the file, the line and what is wrong.
8. In Preferences each row is drawn in its own theme.
9. A theme naming `font=pixel_operator` or another `text_size` draws in it when chosen.
10. `voe_editor --capture` draws the panels in the chosen theme.
