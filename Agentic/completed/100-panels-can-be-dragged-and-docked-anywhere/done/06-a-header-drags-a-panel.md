# 06 — A header drags a panel to dock or float, and a double-click docks it back
folder: editor
after: 05
decisions: 0168, 0439, 0491, 0226, 0282

## Change
0491 points 2 and 4 as a gesture. The outline and ghost come in card 07; the layout is kept by
card 09 (the remember called here writes what it writes today).

- `editor/src/dock.h`: the root gains `voe_ui_rect heads[VOE_EDITOR_CLOSABLE_COUNT]` — last frame's
  header rows in the root's mm, zero size where none was drawn; written by the interface after each
  frame, read by the drag.
- `editor/src/interface.c`: after `voe_ui_frame_end`, each root's `heads` from `closes.header`'s
  node rectangles as `ui` reads them back.
- `editor/src/panel_drag.h` and `.c` (new):
  - `voe_editor_panel_drag` — the pressed closable (`VOE_EDITOR_CLOSABLE_COUNT` none), the press
    point, the panel's rectangle and grab at the press, whether dragging, the current
    `voe_editor_dock_target`, the last press's closable and time, last frame's button.
  - `typedef struct { bool taken; bool ended; } voe_editor_panel_drag_result;`
  - `voe_editor_panel_drag_result voe_editor_panel_drag_frame(voe_editor_panel_drag *drag, voe_editor_dock_root *root, voe_ui_rect area, bool allowed, double now)`
    — on a press edge over a head (floating shown boxes topmost first, then docked; never the bottom
    view's), with `allowed`: remembers it and raises a floating one; a second press on the same
    floating head within `VOE_EDITOR_RESIZE_DOUBLE` is `voe_editor_dock_back`, ended. Held past 1 mm:
    dragging, taken every frame, the target from `voe_editor_dock_target_at`. Release while dragging:
    `voe_editor_dock_move`, ended. A press that never passes 1 mm takes nothing, so the × still clicks.
  - Header: what it owns, the order against the borders, the double-click, 0491 point 4.
- `editor/src/frame_pointer.h` and `.c`: a field `voe_editor_panel_drag moving`, this file's to
  write; step 3 asks it before the seams, with the area and `allowed` the borders get; while it is
  taken, `ui` gets no pointer or press and the seams are not asked, as when the borders take it; an
  end remembers through `voe_editor_panels_remember` with the notice a border drag's end gives. The
  order paragraph says so.
- `editor/src/main.c`: fills `moving` with nothing pressed and no last press.
- `editor/src/src.md`: `panel_drag.h`, `panel_drag.c`; the `frame_pointer.h` and `dock.h` entries.

## Done when
- `grep -q voe_editor_panel_drag_frame editor/src/frame_pointer.c` and
  `grep -q heads editor/src/interface.c` exit 0.
- `d=$(mktemp -d) && XDG_CONFIG_HOME=$d build/debug/editor/voe_editor --capture $d/a.png && test -s $d/a.png`
  exits 0.
