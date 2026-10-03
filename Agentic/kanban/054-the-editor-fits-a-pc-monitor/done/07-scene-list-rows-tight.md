# 07 — The Scene list's rows sit as close as a plain list
folder: editor/src
after: none
decisions: 0168, 0306, 0344

## Change
Today each entity row is spaced by three things stacked: the dock column's `PANEL_GAP` between
every row, the drop rim's `RIM_WIDTH` pad round each row, and the choice's and fold button's
`ui` button pad (2.5 mm times the theme's `spacing`) above and below. Fix it in the Scene list
only; `ui`, `dock.c` and other panels are not touched.

- `editor/src/scene.h`: add `voe_ui_theme list_row` beside `list_dim` and `list_rim`; the comment
  above them gains that `list_row` is the palette at the rows' own spacing, pushed round every
  entity row.
- `editor/src/scene_list.c`:
  - Add `LIST_ROW_SPACING` (0.2f), with a comment: the `spacing` of the theme the entity rows are
    drawn under, so a row's button pad is 0.5 mm, the rim's width, and rows sit a text line apart
    (ADR-0344).
  - `marks_derive`: build `list_row` as a copy of `palette` with `spacing` set to
    `LIST_ROW_SPACING`; build `list_dim` from `list_row` instead of `palette` (same colour
    changes), so a held row keeps its height. `list_rim` is unchanged.
  - `row_draw`: push `list_dim` when `dim`, otherwise `list_row`, and pop it at the end as `dim`
    is popped today (both paths push and pop exactly once).
  - `voe_editor_scene_list_draw`: after Add entity, open one `voe_ui_column_begin` with `.gap = 0`
    and `.across = VOE_UI_ACROSS_FILL` round both row walks (the tree and the flat leftovers),
    closed with `voe_ui_end` after them, so no panel gap sits between rows. The heading and Add
    entity stay where they are, under the palette.
  - `RIM_WIDTH`, `INDENT_PER_DEPTH`, `PREFAB_NAME_GAP` and the drop, hit and drag code are
    unchanged.
  - The file's header comment gains: rows sit in one gapless column under the rows' spacing.
- `editor/src/src.md`: the `scene_list.c` entry names the gapless, tightly padded rows; keep it
  under 300 characters.

## Done when
`grep -c "LIST_ROW_SPACING" editor/src/scene_list.c` prints 2 or more,
`grep -c "list_row" editor/src/scene.h` prints 1 or more, and the folder builds.

The human, on `examples/tank_game`: the Scene list's rows sit about a text line apart, closer
than before, so more entities show; a row is still easy to click and select, the "+"/"-" fold
still folds, dragging a row onto another still shows the rim round the target and the dimmed
held row at the same height, and nested rows are still indented as a tree.
