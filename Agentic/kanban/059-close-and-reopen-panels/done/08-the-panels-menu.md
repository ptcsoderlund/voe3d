# 08 — The Panels menu in the top bar
folder: editor
after: 07
decisions: 0168, 0351, 0363

## Change
0363 point 3. The last card: `## How to test` in `feature.md` is true after it.

- New `editor/src/panels_menu.h` and `panels_menu.c`: the list the Panels button opens.
  - `voe_editor_panels_menu`: `bool open` and one `voe_ui_node` per closable row, recorded as drawn.
  - `void voe_editor_panels_menu_draw(voe_ui_context *ui, voe_editor_panels_menu *menu, voe_ui_rect under,
    const bool open[VOE_EDITOR_CLOSABLE_COUNT])` — an anchored panel hanging below `under` (the Panels
    button's last rectangle), one row per closable in enum order: a fixed-width tick column with √ (U+221A)
    when open, then `voe_editor_closable_name`. Asserts when not open.
  - `voe_editor_closable voe_editor_panels_menu_read(const voe_ui_context *ui, const voe_editor_panels_menu
    *menu)` — the row fired, COUNT for none; and whether the pointer is over the list, for the press outside.
  - The header: what it is, why √ (0363), how it closes.
- `editor/src/topbar.h` and `topbar.c`: a Panels button after Preferences, recorded as `panels_button`;
  `bool voe_editor_topbar_panels_read(...)` like the Preferences read; `voe_editor_topbar` holds the
  `voe_editor_panels_menu`, so it lives across frames. Header's button list names Panels.
- `editor/src/interface.h` and `interface.c`: Panels flips the menu open; the menu is drawn last, over every
  other panel; while it is open the dock gets a pointer with `over` false, as under the other overlays. A
  fired row goes through `voe_editor_panels_toggle` and closes the menu; `escape`, a press outside the list and
  the button, or the browser showing close it. Ticks from `voe_editor_panels_open`. Budget: the Panels button
  and the list (anchor, panel, six rows of button and two labels, "Bottom view" the longest) counted in a
  paragraph, the three numbers raised.
- `editor/src/main.c`: while `bar.menu.open`, the resize is not allowed and the gizmo, the Assets drag and the
  pick are blocked, beside the other panels' guards.
- `editor/src/src.md`: entries for the two new files; `topbar.h` and `interface.c` name Panels.

## Done when
- The folder builds.
- `grep -q '"Panels"' editor/src/topbar.c` exits 0.
- `d=$(mktemp -d) && XDG_CONFIG_HOME=$d build/debug/editor/voe_editor --capture $d/a.png && test -s $d/a.png`
  exits 0.
- The human, in the editor, goes through every step of `## How to test` in
  `Agentic/kanban/059-close-and-reopen-panels/feature.md`, steps 1–8.
