# 03 — The editor's own pads and gaps are 65 %
folder: editor
after: 02
decisions: 0168, 0306, 0344

## Change
Each of these constants becomes its present value times `VOE_EDITOR_SPACING` from
`editor/src/themes.h` (include it where it is not), written as `(<value> * VOE_EDITOR_SPACING)`:
- `editor/src/dock.c`: `PANEL_PAD`, `PANEL_GAP`, `PREVIEW_INSET`. Not `SEAM` (grab area, 0231).
- `editor/src/topbar.c`: `BAR_PAD`, `BAR_GAP`.
- `editor/src/inspector.c`: `COMPONENT_PAD`, `COMPONENT_GAP`, `ROW_GAP`, `LIST_PAD`, `CONTENT_GAP`.
  Not `LIST_BAR`, `SWATCH_WIDE`, `SWATCH_HIGH`.
- `editor/src/add_menu.c`: `LIST_PAD`. Not `LIST_BAR`.
- `editor/src/scene_list.c`: `INDENT_PER_DEPTH`, and the row's literal `.gap = 2.0f`, which gets a
  named constant. Not `RIM_WIDTH`.
- `editor/src/assets_panel.c`: `ASSETS_GAP`.
- `editor/src/browser.c`: `BROWSER_PAD`, `BROWSER_GAP`.
- `editor/src/preferences.c`: `PREFERENCES_PAD`, `PREFERENCES_GAP`. Not the slider's width.
- `editor/src/project_panel.c`: `PROJECT_PANEL_PAD`, `PROJECT_PANEL_GAP`.
- `editor/src/errors.c`: `ERRORS_PAD`, `ERRORS_GAP`.
- `editor/src/drag_ghost.c`: `GHOST_PAD`. Not `GHOST_OFFSET`.
- `editor/src/interface.c`: `PICKER_GAP`.
Panel widths and their minimums in `settings.c` and `dock.c` are not touched.

## Done when
`grep -c "VOE_EDITOR_SPACING"` prints 1 or more for each of the twelve files above, and the folder
builds.

The human, on `examples/tank_game` (feature.md `## How to test`): the text is smaller and the top
bar thinner, more Scene list and Inspector rows fit; the tank's Inspector fields are readable and
nothing clips or overlaps; a dropdown, Add component and Open's browser are tighter and easy to hit;
the text-size slider reads 100 %, 200 % and 50 % work and Reset returns to 100 %; panel widths are
as before and a dragged one is kept; Near black and Near white both get the new sizes; Play shows
the game as before.
