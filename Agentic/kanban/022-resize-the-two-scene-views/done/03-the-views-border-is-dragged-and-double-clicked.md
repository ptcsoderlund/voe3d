# 03 — The views' border is dragged and double-clicked
folder: editor
decisions: 0168, 0226, 0227, 0228, 0229

## Change
The seam between the two views is a border like the side panels': the pointer's shape over it, the drag,
the double-click back to half and half, and the write when it ends.

- `editor/src/resize.c`
  - A border is now any split's node index, not only a held split's; the band, hover and press rules are
    unchanged. The views' split is a COLUMN, so its cursor is already UP_DOWN.
  - The drag writes every split's edge through `voe_editor_dock_split_set` with the pointer less the grab
    as the first child's length, instead of writing a held node's `length` directly; the bar's drag is
    unchanged. The grab is taken against the place's `shown` the same way for every split.
  - The double-click copies both `length` and `fraction` of that node from `voe_editor_dock_default()`,
    so the views' border goes back to 0.5 and only that border changes.
  - The file header's first line and any comment that says "held split" say "split".
- `editor/src/resize.h` — the header's opening names the views' seam among the borders; the border
  paragraph says any split's node index; the double-click paragraph says the views go back to an even
  split (0228).
- `editor/src/src.md` — the `resize.h` entry names the views' border.

Read `resize.h`, `resize.c`, and `dock.h`'s declarations of the arrangement and `voe_editor_dock_split_set`;
no other file. `main.c` needs no change: it already calls the frame, sets the cursor and remembers on
`ended`.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `editor` exits 0, and `voe_editor --capture` as in card
02 still writes its picture with no file and with one.
