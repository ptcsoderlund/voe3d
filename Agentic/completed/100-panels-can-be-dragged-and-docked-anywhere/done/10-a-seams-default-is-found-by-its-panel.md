# 10 — A seam's double-click finds its default by its panel
folder: editor
after: 09
decisions: 0168, 0439, 0491, 0226, 0228
read: feature.md

## Change
0491 point 8. Since card 05 a moved tree's node indices are not the default tree's, so a seam's
double-click must not look the default up by index.

- `editor/src/resize.c`: the double-click's default length is found by what the split holds: the
  views' split goes to a half (0228); a split holding a leaf of a panel, or a split whose first leaf
  is one, takes `voe_editor_dock_panel_length` of that panel in `voe_editor_dock_default()`; a panel
  the default tree holds no length for (Frame, Project, Errors, Landscape) takes
  `VOE_EDITOR_DOCK_SIDE_WIDE`. The written length goes through `voe_editor_dock_split_set` as now.
- `editor/src/resize.h`: the double-click paragraph says the default is found by panel, not node.
- `editor/src/src.md`: the `resize.c` entry, if it names how the default is found.

## Done when
- `grep -q voe_editor_dock_panel_length editor/src/resize.c` exits 0.
- `d=$(mktemp -d) && XDG_CONFIG_HOME=$d build/debug/editor/voe_editor --capture $d/a.png && test -s $d/a.png`
  exits 0.
- The human, in the editor, walks `## How to test` in `feature.md`, steps 1 to 8.
