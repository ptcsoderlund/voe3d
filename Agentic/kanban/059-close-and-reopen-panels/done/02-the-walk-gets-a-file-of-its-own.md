# 02 — The dock's walk gets a file of its own
folder: editor
after: 01
decisions: 0168

## Change
`editor/src/dock.c` (742 lines) is the file the next cards grow; split it by function first. No behaviour
changes and `editor/src/dock.h` keeps every declaration it has.

- New `editor/src/dock_walk.c`: `voe_editor_dock_walk`, `voe_editor_panel_draw` and the static helpers only
  they use (the walk's node emission, seam drawing, the leaf panels and their scroll areas, the camera preview).
- `dock.c` keeps the tree: the default tree, `voe_editor_dock_arrange`, the lengths, the share,
  `voe_editor_dock_shows_view`, `voe_editor_dock_over_panel`.
- A helper both files need (e.g. the one turning a size into the pair read against the parent's flow) is
  declared once in `dock.h` under a short section saying it is dock.c's and dock_walk.c's own, not for callers.
- Headers: `dock.c`'s header keeps the arrangement's paragraphs; the walk's paragraphs (fixed sizes, the seam
  drawn, the wrapping row, the scene view's padding, every leaf a scroll area, the one surface role) move to
  `dock_walk.c`'s header.
- `editor/src/src.md`: the `dock.c` entry says what stays; a new `dock_walk.c` entry.

## Done when
- The folder builds.
- `test $(wc -l < editor/src/dock.c) -lt 700 && test $(wc -l < editor/src/dock_walk.c) -lt 700` exits 0.
- `grep -q '^void voe_editor_dock_walk' editor/src/dock_walk.c` exits 0.
- `d=$(mktemp -d) && XDG_CONFIG_HOME=$d build/debug/editor/voe_editor --capture $d/a.png && test -s $d/a.png`
  exits 0.
