# 12 — The Rendering submenu opens on the side it fits and stays open
folder: editor
decisions: 0168, 0199, 0200, 0221

## Change
Bug 01: with the Inspector at the window's right edge, "Rendering >" opens its submenu to the right, partly
outside the window, and the submenu closes by itself within a fraction of a second. Both faults are in
`editor/src/inspector_edit.c`, which owns where a submenu sits and when the menu closes; fix them there,
once, for every level. Open `editor/src/inspector_edit.h`, `inspector_edit.c`, and the headers of
`editor/src/inspector.h` and `editor/src/add_menu.h` for the menu's state and the drawn lists; no other file.

- `submenu_place` — the side test. Card 08 meant: right of `from` when the submenu's whole width fits between
  `from`'s right edge and the right edge of the room it may use; else its right edge at `from`'s left edge.
  Find why that test passes at the window's edge. Suspects, in order: `from`'s edges and the room compared in
  different spaces (the content column's against the surface's millimetres, as `overlay_place`'s comment
  warns); the room taken as the content column or scroll content rather than the panel's visible area; the
  first frame's "not drawn yet, to the right" never measured again. The fixed rule: right only when it fits,
  else left; wholly inside the panel's visible area either way, and so inside the window.
- `voe_editor_inspector_buttons_read` — the closing. After a group opens, the only things that close the menu
  or a submenu are: a type row fired (adds it and closes all), another group row fired (closes that level and
  deeper), a press outside every open list and the button, Escape (`voe_editor_inspector_add_close`), or a
  drawn entity not the one it opened for. Find which rule closes it on the frames after the open with nothing
  pressed. Suspects: the outside test reading `down` as a level instead of the press edge, so the press that
  fired the group row is still "down" next frame over a submenu not yet placed; a submenu's visible rectangle
  empty because it was clipped away (fault 1) and so every pointer counted as outside; the open dropdown's
  "open and drew no rows" close applied to the menu or a submenu; a group row firing twice and so toggling
  shut. Fix the rule that does it, not by special-casing Rendering or level one. If more than one holds, fix
  each.
- `editor/src/inspector_edit.h` — the header's submenu sentences and `buttons_read`'s comment say: which room
  the side test measures and in which space; that the outside test is the press edge; the full list of what
  closes the menu (0221). Only where they are wrong or silent now.
- `editor/src/src.md` — the `inspector_edit.c` entry, only if what it lists changed.

If the cause is plainly outside `editor` (a `ui` rect, clip or action that does not say what its header
promises), stop and block the card naming the `ui` header and the promise.

## Done when
The coder: `bash ~/.claude/skills/checks/scripts/checks.sh --all` exits 0.

The human, at a running `voe_editor` on a project with the Inspector at the window's right edge, follows bug
01's steps: Add entity, Add component, "Rendering >". The submenu opens to the left of the list, wholly in the
window, and stays open with the pointer still. Moving the pointer over it and back leaves it open. Picking
Shape adds a shape and closes the menu; reopening and picking Light adds a light. With the menu open, a press
outside it closes it, and so does Escape.
