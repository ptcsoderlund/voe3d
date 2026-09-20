# 17 — The monochrome editor and its sliders are walked through
folder: editor
decisions: 0168, 0177, 0194, 0197
read: feature.md

## Change
The editor's own work for this feature is already in the tree: commit cf0b806 put the Preferences
result block in `editor/src/interface.c` — `VOE_EDITOR_PREFERENCES_ADJUST` deriving the chosen entry's
palette again where it stands, `VOE_EDITOR_PREFERENCES_RESET` putting the theme's own two back, and
`voe_editor_themes_scalars_write` when `!result.sliding` — and the Preferences and themes paragraphs of
`editor/editor.md`. This card writes nothing by default; it is the walk that proves the feature, and
whatever that walk shows to be wrong.

Run the nine steps of `feature.md`'s `## How to test` in a built `voe_editor`. A step that does not
hold is fixed here, in this folder and in at most one of `editor/src/interface.c`,
`editor/src/preferences.c` and `editor/src/themes.c` — read the header of the one you touch first
(`editor/src/preferences.h`, `editor/src/themes.h`, `editor/src/theme_scalars.h`) and no other file. A
fault that is plainly `ui`'s or `theme`'s is not fixed here: say so and the card is blocked.

For step 4, make `~/.config/voe3d/themes/amber.theme` before the walk, these six lines and no more:

	[Amber]
	hue="#D4A02B"
	contrast_strength=1.0
	surface_separation=1.0
	mode=dark
	text_size=4.0

To look at a frame without a
person at the screen, `voe_editor --capture <path.png> --size 1280x720` draws one frame with no window
and writes it (ADR-0177); that is how a drawing fault is pinned down, not how the walk is passed.

## Done when
The coder: `checks.sh --all` exits 0.

The human, at a running `voe_editor`, sees every one of `feature.md`'s nine steps hold — the selected
`Scene` row, a held button and a dragged number box marked by inversion and grey alone in Near black
and Near white; `amber.theme` turning the whole editor one amber hue with the same marks; that file
with `accent=` instead still drawing and showing a notice naming the file and the line; both sliders
moving text and surfaces as they are dragged, readable at either end; the theme and both slider
positions as they were left after a restart, with `amber.theme` unchanged on disk; and Near black
showing its own two values, which Reset puts back.
