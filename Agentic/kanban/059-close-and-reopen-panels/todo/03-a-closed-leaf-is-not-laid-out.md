# 03 — A closed dock leaf is not laid out
folder: editor
after: 02
decisions: 0168, 0351, 0363

## Change
0363 point 1. Nothing closes a panel yet (card 05 does); with every flag open the editor looks as before.

- `editor/src/dock.h`:
  - `voe_editor_closable` enum, in menu order: SCENE_LIST, ASSETS, INSPECTOR, BOTTOM_VIEW, PROJECT, ERRORS,
    COUNT; `VOE_EDITOR_CLOSABLE_DOCKED` (4): the first four are dock leaves.
  - `const char *voe_editor_closable_name(voe_editor_closable which)` — "Scene list", "Assets", "Inspector",
    "Bottom view", "Project", "Errors".
  - `voe_editor_closable voe_editor_dock_leaf_closable(const voe_editor_dock_node *leaf)` — the leaf's
    closable, COUNT for the top view (a SCENE_VIEW leaf of view 0).
  - `voe_editor_dock_root` gains `bool closed[VOE_EDITOR_CLOSABLE_DOCKED]`; zeroed is all open. Its comment
    says it is apart from the tree on purpose (0351).
  - `voe_editor_dock_arrange` gains `const bool closed[VOE_EDITOR_CLOSABLE_DOCKED]` after the tree.
  - `voe_editor_dock_place` gains `bool laid` (the node has a rectangle) and `bool seamed` (a split with both
    children laid; only then are `seam`, `least`, `most` and `shown` meaningful).
  - `voe_editor_dock_shows_view` takes `const voe_editor_dock_root *` instead of the tree.
- `editor/src/dock.c`: a closed leaf is not laid; a split with one laid child gives it the split's whole
  rectangle and no seam; with none it is not laid itself. `length` and `fraction` are never written by this.
  `voe_editor_dock_over_panel` is false for a leaf not laid; `shows_view` false for a closed bottom view.
- `editor/src/dock_walk.c`: a node not laid emits nothing; a split with one laid child emits only that child at
  the split's size; every view the walk does not show gets its `rect` zeroed so `voe_editor_views_under`
  (view.h) misses it.
- `editor/src/resize.c`: passes the root's `closed` to arrange; only a `seamed` split is a border.
- `editor/src/view_passes.h` and `view_passes.c`: the tree parameter becomes the root, for `shows_view`.
- `editor/src/main.c`: its `shows_view` and view-pass calls pass `&roots[0]`.
- Headers: dock.h's "nothing is closable" line in its header now says a leaf can be closed and is then not laid
  out (0363); view.h's "a view whose leaf is not in the tree" paragraph also covers a closed one — read
  `editor/src/view.h`'s header and amend that paragraph only. `editor/src/src.md`: the `dock.h` entry mentions
  closed leaves.

## Done when
- The folder builds.
- `grep -q 'bool closed\[VOE_EDITOR_CLOSABLE_DOCKED\]' editor/src/dock.h` and
  `grep -q seamed editor/src/resize.c` exit 0.
- `d=$(mktemp -d) && XDG_CONFIG_HOME=$d build/debug/editor/voe_editor --capture $d/a.png && test -s $d/a.png`
  exits 0.
