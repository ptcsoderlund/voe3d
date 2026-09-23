# 07 — Add component opens a list over the panel
folder: editor
decisions: 0168, 0199, 0200, 0217, 0221

## Change
Add component stops showing a flat row of buttons and opens the top level of `add_menu.h`'s tree as a list
that behaves like the named-field dropdown's open list. Groups are drawn but open nothing yet (card 08).

- `editor/src/inspector.h` — `choices`, `choice_count` and `choosing` give way to the menu's state, kept across
  frames: whether it is open and the entity it opened for; per frame: the built `voe_editor_add_menu`, its
  list panel and rows area, and each drawn row's node with its entry index. Header: the Add component
  paragraph says it opens a list and how it closes; the constants that sized `choices` go if unused.
- `editor/src/add_menu.h` / `add_menu.c` — gain the draw: the list for entries whose parent is
  `VOE_EDITOR_ADD_MENU_TOP`, one row per entry, a group's label followed by a marker saying it opens further.
  Draw it the way `dropdown_list` in `editor/src/inspector.c` draws the open list: an anchored child of the
  content column emitted after every section, a panel that takes the pointer, rows in a scroll area capped by a
  height (0199, 0200). Mirror that function; do not change it.
- `editor/src/inspector.c` — `add_component` draws one "Add component" button, builds the menu when open and
  calls the draw after the sections, beside the open dropdown's.
- `editor/src/inspector_edit.h` / `inspector_edit.c` — `voe_editor_inspector_buttons_read`: the button toggles
  the menu, and opening it closes the dropdown and the picker (`scene.h`); a leaf row fired adds its type
  through `voe_editor_entities_component_add`, counted as Add component is today, and closes the menu; a press
  outside the list's visible rectangle and the button closes it; a frame whose drawn entity is not the one it
  opened for closes it. The list is placed every frame by the side-and-cap rule the open dropdown uses: lift
  that arithmetic out of `buttons_read` into one function declared in `inspector_edit.h`, used by both, not a
  copy. Header follows.
- `editor/src/interface.c` — where Escape closes the open dropdown, it closes the menu too.
- `editor/src/src.md` — entries for the files whose contents changed.

## Done when
`grep -n "choosing" editor/src/*.[ch]` prints nothing, and the Checks line of `CLAUDE.md` with `{folder}` =
`editor` exits 0.
