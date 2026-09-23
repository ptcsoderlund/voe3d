# 05 — The text size slider is walked through
folder: editor
decisions: 0168, 0197, 0219, 0224, 0225
read: feature.md

## Change
No product code by default: this is the walk that proves the feature, and whatever it shows to be wrong.

The human walks `feature.md`'s eight steps at a running `voe_editor <scratch>/walk` (a New project with an
entity whose Inspector has several fields) and reports what does not hold. A failing step is fixed here, in
at most one of `editor/src/preferences.c`, `themes.c`, `theme_scalars.c`, `topbar.c`, `inspector.c`,
`scene.c`, `add_menu.c`, `browser.c` and `dock.c`; read the header of the one you touch and no other
file. Text cut or drawn over a neighbour is fixed by letting the element take its natural size, wrap or sit
in the leaf's scroll area, never by a second scale on rows. A fault plainly `ui`'s (a label measured wrong
for its theme's `text_size`, a slider or number box whose fixed height cuts its text, an overlay that does
not fit, `ui/include/ui/widgets.h`, `slider.h`, `layout.h`) is not fixed here: the card is blocked, naming
it.

## Done when
The coder: `bash ~/.claude/skills/checks/scripts/checks.sh --all` exits 0.

The human, at a running `voe_editor`, sees every step of `feature.md` hold: the Text size slider sits with
the others at 100%; at 200% the top bar, Scene list, Inspector, menus and dropdowns grow as it is dragged,
rows and buttons taller, nothing cut or overlapping; a long field name in a narrow Inspector stays in its
column and panel; at 50% everything shrinks and stays readable; another theme has its own size and keeps a
different one; switching back restores the first; Reset puts 100% back with the other sliders; after a
restart each theme has the size it was given.
