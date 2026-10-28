# 08 — A floating panel is resized by its edges and takes the pointer
folder: editor
after: 07
decisions: 0168, 0439, 0491, 0226

## Change
0491 points 3 and 6.

- `editor/src/dock.h` and `dock.c`:
  - `voe_editor_closable voe_editor_dock_float_under(const voe_editor_dock_root *root, voe_ui_rect area, voe_math_float2 point)`
    — the topmost shown floating box (not closed) holding `point`, `VOE_EDITOR_CLOSABLE_COUNT` for none.
  - `voe_editor_dock_over_panel`: true over the panel's shown floating box; false over its laid leaf
    where another panel's floating box covers the point. Its comment says so. Its callers
    (`assets_drag.c`, `frame_pointer.c`, `interface_read.c`) are unchanged.
- `editor/src/float_edges.h` and `.c` (new):
  - `voe_editor_float_edges` — the held closable (`VOE_EDITOR_CLOSABLE_COUNT` none), which of its
    four edges are held, the box and pointer at the press, last frame's button.
  - `typedef struct { voe_platform_cursor cursor; bool taken; bool ended; } voe_editor_float_edges_result;`
  - `voe_editor_float_edges_result voe_editor_float_edges_frame(voe_editor_float_edges *edges, voe_editor_dock_root *root, voe_ui_rect area, bool allowed)`
    — hover and a press edge within `VOE_EDITOR_RESIZE_REACH` of a shown floating box's edge, the
    topmost box first, two edges at a corner; the drag writes that float's `box` (from the area's
    top left) at least 30 × 20 mm and inside the area; release ends. Cursor LEFT_RIGHT for a side
    edge, UP_DOWN for a top or bottom one; the press raises the box.
  - Header: what it owns and why it is not the seams' file (a box is not a split), 0491 point 3.
- `editor/src/frame_pointer.h` and `.c`: a field `voe_editor_float_edges edges`, this file's; step 3
  asks it first, then the panel drag, then the seams; taken as the borders' is; its cursor wins while
  it has the pointer; an end remembers as card 06's does. While `voe_editor_dock_float_under` finds a
  box at the pointer, the fly does not start, and no seam, middle drag, brush, gizmo, Assets drag or
  pick starts or picks under it; `ui` keeps the pointer. The order paragraph says both.
- `editor/src/main.c`: fills `edges` with nothing held.
- `editor/src/src.md`: `float_edges.h`, `float_edges.c`; the `frame_pointer.h` and `dock.h` entries.

## Done when
- `grep -q voe_editor_float_edges_frame editor/src/frame_pointer.c` and
  `grep -q voe_editor_dock_float_under editor/src/frame_pointer.c` exit 0.
- `d=$(mktemp -d) && XDG_CONFIG_HOME=$d build/debug/editor/voe_editor --capture $d/a.png && test -s $d/a.png`
  exits 0.
