# 02 — A split holds a side panel at a length
folder: editor
decisions: 0168, 0226

## Change
The dock's side panels become lengths in millimetres, and the dock can say where every node and seam is
without a `ui` frame. Nothing drags yet (card 04).

- `editor/src/dock.h`
  - `voe_editor_dock_hold` enum: `_FRACTION` (zero, so every existing initialiser keeps its meaning),
    `_FIRST`, `_SECOND`. `voe_editor_dock_node` gains `hold` and `float length`, read by a SPLIT: which
    child is held at `length` mm, the other taking the rest; `fraction` is read only under `_FRACTION`.
  - `VOE_EDITOR_DOCK_PANEL_MIN` 30.0f, `VOE_EDITOR_DOCK_VIEW_ROOM` 40.0f, `VOE_EDITOR_DOCK_SIDE_WIDE`
    48.0f, each with the reason from 0226.
  - `voe_editor_dock_arrangement`: per node its `voe_ui_rect rect`; per split its `voe_ui_rect seam` (the
    gap between the two children) and, when held, `float least`, `most` (the bounds a drag may write) and
    `shown` (the length laid out). `void voe_editor_dock_arrange(const voe_editor_dock_tree *tree,
    voe_ui_rect area, voe_editor_dock_arrangement *out)`.
  - `float voe_editor_dock_panel_length(const voe_editor_dock_tree *tree, voe_editor_panel panel)` and
    `void voe_editor_dock_panel_length_set(voe_editor_dock_tree *tree, voe_editor_panel panel,
    float length)`: the length of the split whose held child is a leaf of `panel`; nought / nothing when
    no split holds one.
  - Header: the "fraction is the number a splitter will later write" paragraphs become: a held length is
    what a drag writes, through resize.h, and the tree is still only numbers; `_default`'s comment says
    Scene and Inspector are held at `SIDE_WIDE` and the views take the rest.
- `editor/src/dock.c`
  - `voe_editor_dock_default`: the root ROW split holds FIRST (Scene) at `SIDE_WIDE`; the split under it
    holds SECOND (Inspector) at `SIDE_WIDE`; the views' column stays at 0.5.
  - `voe_editor_dock_arrange` divides the way the walk does today (the gap first) and is what the walk now
    takes every child's length from, so the two cannot disagree. A held length is clamped as 0226 says:
    what a subtree needs along an axis is `VIEW_ROOM` for a scene view leaf, `PANEL_MIN` for another leaf,
    along a split the children's needs plus the gap (a held child needing its own `shown`), across one
    the larger; `least` = the held child's need, `most` = the length less the gap less the other side's
    need; `shown` = `length` in `least..most`, `most` winning, never below nought.
  - The two panel-length calls; the file header's "nothing hit-tests the gap" and "a fraction of a known
    length is the number a drag will write" now name the held length and resize.h.
- `editor/src/src.md` — the `dock.h` and `dock.c` entries name held lengths and the arrangement.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `editor` exits 0. With `XDG_CONFIG_HOME` at an empty
scratch folder, `voe_editor <scratch>/p --capture <scratch>/a.png --size 1280x720` shows the Scene list
and Inspector each about a fifth of the width, as before; with `--size 2560x720` they are the same width in
millimetres as at 1280x720 (so about a tenth of the picture) and the views take the rest.
