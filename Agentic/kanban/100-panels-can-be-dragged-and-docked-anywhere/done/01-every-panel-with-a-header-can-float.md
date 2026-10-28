# 01 — Every panel with a header is a closable that can float
folder: editor
after: none
decisions: 0168, 0439, 0491, 0363

## Change
0491 points 1, 2, 3, 5 as data. Nothing floats or moves yet and the editor looks as before: the four
tool panels still draw as today's overlays (card 03 moves them).

- `editor/src/dock.h`:
  - `voe_editor_panel` gains FRAME, PROJECT, ERRORS, LANDSCAPE before COUNT.
  - `voe_editor_closable` gains LANDSCAPE after FRAME; `VOE_EDITOR_CLOSABLE_LISTED` (7), the Panels
    list's rows. `voe_editor_closable_name` answers "Landscape".
  - `voe_editor_dock_leaf_closable` maps the four new panels to their closables.
  - `VOE_EDITOR_DOCK_NODES` 32.
  - `typedef struct { bool floating; voe_ui_rect box; uint32_t raised; } voe_editor_dock_float;` —
    `box` in mm from the dock area's top left, as set; larger `raised` drawn later.
  - `VOE_EDITOR_DOCK_FLOAT_LOW` 20 mm, a box's least height (its least width is PANEL_MIN).
  - `voe_editor_dock_root`: `closed` widened to `[VOE_EDITOR_CLOSABLE_COUNT]`, the entries past the
    docked four filled by the caller each frame from each panel's own `showing`, like `pointer`;
    gains `voe_editor_dock_float floats[VOE_EDITOR_CLOSABLE_COUNT]`.
  - `void voe_editor_dock_floats_default(voe_editor_dock_float floats[VOE_EDITOR_CLOSABLE_COUNT])` —
    the four tool panels floating at 0491 point 3's boxes, the rest docked; `raised` by closable order.
  - `voe_ui_rect voe_editor_dock_float_shown(const voe_editor_dock_float *f, voe_ui_rect area)` — the
    box clamped as 0491 point 3 says, in the root's mm (`area` is the dock area there).
  - `voe_editor_dock_arrange(const voe_editor_dock_root *root, voe_ui_rect area, out)` replaces the
    tree and `closed` parameters: a leaf whose closable is closed or floating is not laid.
  - Header: the "NOTHING HERE IS REARRANGEABLE" paragraph is rewritten: panels move (0439), a floating
    leaf stays in the tree not laid (0491 point 2); the `closed` paragraph says which entries the
    caller fills.
- `editor/src/dock.c`: the above; the switch over panels covers the four (no leaf of them exists yet).
- `editor/src/dock_walk.c`, `editor/src/resize.c`: arrange called with the root.
  `voe_editor_panel_draw`'s switch gains the four cases, drawing nothing until card 03.
- `editor/src/panels.c`: start fills `root->floats` with the default.
- `editor/src/panels_menu.c` and the `open[]` loop in `editor/src/interface.c`: rows up to
  `VOE_EDITOR_CLOSABLE_LISTED`.
- `editor/src/main.c`: each frame before `voe_editor_frame_pointer_read`, `roots[0].closed` for
  PROJECT, ERRORS, FRAME and LANDSCAPE set from `project_panel`, `session.errors`, `breakdown` and
  `landscape_panel`'s `showing` (closed when not showing).
- `editor/src/src.md`: the `dock.h` entry names floating panels.

## Done when
- `grep -q VOE_EDITOR_CLOSABLE_LISTED editor/src/panels_menu.c` and
  `grep -q voe_editor_dock_float_shown editor/src/dock.h` exit 0.
- `d=$(mktemp -d) && XDG_CONFIG_HOME=$d build/debug/editor/voe_editor --capture $d/a.png && test -s $d/a.png`
  exits 0.
