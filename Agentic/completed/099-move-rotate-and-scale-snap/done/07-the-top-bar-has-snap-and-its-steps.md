# 07 — The top bar has Snap and its steps
folder: editor
after: 06
decisions: 0168, 0482, 0438, 0225

## Change
0482 points 5 and 7: after the gizmo's mode label, a Snap choice and the three steps as number
boxes, changing `scene->snap` and remembering it.

- `editor/src/topbar.h` —
  - `voe_editor_topbar` gains the Snap node, three step box nodes and which boxes were held last
    frame;
  - `voe_editor_topbar_draw` gains `const voe_editor_snap *snap`, drawn after the mode label:
    `voe_ui_choice_begin` "Snap" selected while on, then boxes showing "0.5 m", "15°", "0.1"
    (fewest digits that show the value), dragged at 0.05 m, 1° and 0.01 per millimetre;
  - `bool voe_editor_topbar_snap_read(const voe_ui_context *ui, voe_editor_topbar *bar,
    voe_editor_snap *snap)` — in the clicks' window: a fired Snap flips `on`; a changed box
    writes its step clamped to settings.h's range; true when the result should be remembered:
    a flip, a typed commit, or the frame a box's drag ends.
  Header: the bar's list of what it draws; the steps change only the gizmo's drags, never a
  typed Inspector value.
- `editor/src/topbar.c` — the above.
- `editor/src/interface.c` — the draw passes `&scene->snap`; where the bar's clicks are read,
  `voe_editor_topbar_snap_read`, and on true `voe_editor_settings_snap_write`, a failure said in
  the session's notice as a panel toggle's is ("editor_settings").
- `editor/src/interface.h` — a paragraph counting the bar's new nodes and elements (a choice and
  its label, three boxes and their figures), and `VOE_EDITOR_INTERFACE_NODES` / `_ELEMENTS`
  grown by them.
- `editor/src/src.md` — the `topbar` and `interface` entries follow.

## Done when
The editor builds, `grep -n "voe_editor_topbar_snap_read" editor/src/interface.c` prints a line,
and `build/debug/editor/voe_editor examples/capsule --capture <a scratch file>.png` (options.h's
form) writes a picture whose top bar the coder opens and sees Snap and the three steps in.
