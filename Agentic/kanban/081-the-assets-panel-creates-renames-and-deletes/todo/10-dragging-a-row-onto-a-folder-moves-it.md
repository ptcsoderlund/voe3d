# 10 — Dragging a row onto a folder moves it
folder: editor
after: 09
decisions: 0168, 0377, 0378

## Change
Any row, a folder's too, dragged onto a folder row or onto Up moves there (0377 point 3).

- `editor/src/assets_panel.h` / `.c` — every row may be held, not only a model, prefab or picture row;
  `held` says which kind. A folder row held and dragged is not entered on release. The read can say
  which folder row, or Up, a point is over (extend `voe_editor_assets_row_at`).
- `editor/src/assets_drag.h` / `.c` — a sixth outcome: released over a folder row other than the dragged
  row, or over Up, the row moves there through `voe_editor_assets_move` (assets_manage.h); the ghost is
  refused (drag_ghost.h) over the dragged row itself, a folder inside a dragged folder, and everywhere a
  release would do nothing. A folder or a file that only lists starts a drag that only moves; the
  existing outcomes for models, prefabs and pictures stay as they are.
- `editor/src/frame_pointer.c` — passes what the drag now needs, if anything.
- Headers of the files touched say the new outcome; `editor/src/src.md` entries updated.

## Done when
`grep -c voe_editor_assets_move editor/src/assets_drag.c` prints 1 or more, and the folder's check passes.
Human: How to test steps 4, 5 and 7 in feature.md, and a row dragged onto Up.
