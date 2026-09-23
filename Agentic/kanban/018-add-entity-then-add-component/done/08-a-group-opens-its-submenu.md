# 08 — A group opens its submenu beside it
folder: editor
decisions: 0168, 0199, 0200, 0217, 0221

## Change
A group row in the Add component list opens the list of its children beside it, to any depth the tree has.

- `editor/src/inspector.h` — the menu's kept state gains the open groups, one entry index per level
  (`VOE_EDITOR_ADD_MENU_DEPTH`), and a count; the per-frame part gains one list panel, rows area and rows per
  open level.
- `editor/src/add_menu.h` / `add_menu.c` — the draw emits, after the top list, one list per open group with
  that group's children, in the same shape as card 07's. Header: submenu placement (0221) and that every open
  list is solid over its outline (0199).
- `editor/src/inspector_edit.c` — the read: a group row fired opens its group at its level and closes every
  open group at that level or deeper; a leaf at any level adds and closes the whole menu; a press closes the
  menu only when it lands outside every open list and the button. Placement, every frame: a submenu's left
  edge at its list's right edge and its top at its group row's top; when the panel's area has no room on the
  right, its right edge at its list's left edge instead; vertically, the side-and-cap function from card 07
  with the row as the widget. `inspector_edit.h` header follows.
- `editor/src/src.md` — the `add_menu.c` entry if its contents grew.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `editor` exits 0. The human, in 09's walk, opens Rendering
and sees Light and Shape beside it.
