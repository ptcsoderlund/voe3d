# 09 — Right-click opens Create and the row menu
folder: editor
after: 08
decisions: 0168, 0377, 0378

## Change
The right button over the Assets panel opens a menu (0377 point 2, 0378 point 8).

- New `editor/src/assets_menu.h` / `.c` — the menu as an anchored panel at the pointer, drawn and read
  the way `editor/src/panels_menu.h` / `.c` do theirs, rows as wide as the widest. Over a row: Rename,
  Duplicate, Delete. Over the empty part: Create, which opens a submenu beside it, placed by
  `editor/src/inspector_place.h`'s rule, holding Folder; the header says later kinds add their row there
  (0377 point 2). A fired row closes the menu; Escape or a press outside closes it.
- `editor/src/assets_panel.h` / `.c` — `voe_editor_assets_row_at(assets, point)`: the row under a point
  by last frame's rectangles, or none, or the empty part, or outside the panel.
- `editor/src/frame_pointer.c` — the right button's down edge (platform/input.h), when no view flies
  and the pointer is over the panel, selects the row under it, gives the panel the keyboard, and opens
  the menu there.
- `editor/src/interface.h` / `.c` — the menu's state held, drawn over everything as the Panels list is,
  read after the frame: Rename calls `voe_editor_assets_rename_begin`, Delete
  `voe_editor_assets_delete_begin`, Folder `voe_editor_assets_folder_begin` (assets_panel.h), Duplicate
  `voe_editor_assets_duplicate` (assets_manage.h). Escape's order closes the menu first.
- `editor/src/src.md` — the new files' entries; headers of the files touched updated.

## Done when
`test -f editor/src/assets_menu.h && grep -c voe_editor_assets_folder_begin editor/src/interface.c`
prints 1 or more, and the folder's check passes.
Human: How to test steps 1, 2, 3, 6 and 8 in feature.md.
