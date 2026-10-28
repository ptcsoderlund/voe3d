# 07 — A panel drag shows its outline and its ghost
folder: editor
after: 06
decisions: 0168, 0439, 0491, 0232, 0285

## Change
0491 point 4: while a panel is dragged, where it would go is outlined and its name follows the pointer.

- `editor/src/dock.h`: the root gains `voe_editor_closable moving` (`VOE_EDITOR_CLOSABLE_COUNT` for
  none) and `voe_ui_rect outline` in the root's mm — the caller's to fill each frame, like `lit`.
  The root's comment says so.
- `editor/src/frame_pointer.c`: after the panel drag, `root->moving` is the dragged closable while
  dragging, else none, and `root->outline` its target's outline.
- `editor/src/dock_walk.c`: while `moving` is set, last in the dock's container, the outline as four
  anchored strips along its edges drawn as the lit seam's two stripes (`inverse_ink` beside
  `inverse`), nothing inside filled and no pointer taken. Header: a paragraph on the outline.
- `editor/src/interface.c`: while a root's `moving` is set, `voe_editor_drag_ghost_draw` with
  `voe_editor_closable_name` of it, never refused, at the pointer, as the other drags' ghosts are.
- `editor/src/interface.h`: the outline's strips counted on top in a paragraph like the others, the
  numbers raised; the ghost is the one already counted (one drag at a time).
- `editor/src/src.md`: the `dock_walk.c` entry.

## Done when
- `grep -q outline editor/src/dock_walk.c` and `grep -q voe_editor_closable_name editor/src/interface.c`
  exit 0.
- `d=$(mktemp -d) && XDG_CONFIG_HOME=$d build/debug/editor/voe_editor --capture $d/a.png && test -s $d/a.png`
  exits 0.
