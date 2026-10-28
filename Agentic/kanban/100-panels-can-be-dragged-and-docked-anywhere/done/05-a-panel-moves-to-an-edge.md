# 05 — A panel moves to an edge of the dock, the views or another panel
folder: editor
after: 04
decisions: 0168, 0439, 0491, 0226

## Change
0491 points 2 and 4 as tree arithmetic, in the root's millimetres; `area` is the dock area there and a
float's box is stored from `area`'s top left. Nothing calls these yet (card 06).

- `editor/src/dock.h`:
  - `voe_editor_dock_edge`: LEFT, RIGHT, TOP, BOTTOM.
  - `voe_editor_dock_target_kind`: FLOAT, AT_DOCK, AT_VIEWS, AT_PANEL.
  - `voe_editor_dock_target` — kind, `voe_editor_closable panel` (AT_PANEL's), edge, `float length`
    (the held length a dock gets), `voe_ui_rect outline` (where it would go).
  - `voe_ui_rect voe_editor_dock_panel_rect(const voe_editor_dock_root *root, voe_ui_rect area, voe_editor_closable which)`
    — its shown floating box, else its laid leaf's rectangle, else zero size.
  - `voe_editor_dock_target voe_editor_dock_target_at(const voe_editor_dock_root *root, voe_ui_rect area, voe_editor_closable dragged, voe_ui_rect box, voe_math_float2 grab, voe_math_float2 pointer)`
    — 0491 point 4's first match: `box` is the dragged panel's rectangle at the press, `grab` the
    pointer's offset in it. A dock's `length` is the box's size along the edge's axis; its outline
    the target's strip that deep, at most the target's extent; a float's outline its box at
    `pointer - grab`, clamped as `voe_editor_dock_float_shown` clamps.
  - `void voe_editor_dock_move(voe_editor_dock_root *root, voe_ui_rect area, voe_editor_closable which, const voe_editor_dock_target *target)`
    — FLOAT: floating, the box set from the outline, raised above every other. Otherwise the panel's
    leaf taken out if there (its parent split replaced by the sibling), then a new split in place of
    the target, found after the removal (the dock's root, the views' split, the panel's laid leaf; the
    dock's right edge when not laid): ROW for LEFT and RIGHT, the panel held first for LEFT and TOP,
    second else, at `length`; no longer floating; the tree compacted to the nodes its root reaches.
  - `void voe_editor_dock_back(voe_editor_dock_root *root, voe_ui_rect area, voe_editor_closable which)`
    — a floating panel with a leaf stops floating; one without moves to its 0491 point 2 default.
  - Header: a paragraph on moving — what is removed and inserted, targets by identity not index,
    compaction (0491).
- `editor/src/dock_move.c` (new, as `dock_walk.c` is beside `dock.c`, which stays under 800 lines):
  the five, with a file header comment; any walk over the tree bounded by `VOE_EDITOR_DOCK_DEPTH`.
- `editor/src/src.md`: the `dock.h` entry, and a `dock_move.c` entry.

## Done when
`grep -q voe_editor_dock_target_at editor/src/dock_move.c` and `grep -q voe_editor_dock_back editor/src/dock_move.c`
exit 0.
